// WavPcm::extract() 的单元测试。
//
// 被测函数是纯自由函数（只依赖 QtCore），可以在构建机上以桌面目标直接跑，
// 见 tests/CMakeLists.txt。
//
// 运行方式（构建机上）：
//   cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug
//   cmake --build build-tests --target tst_wavpcm
//   ./build-tests/tests/tst_wavpcm

#include <QtTest>
#include <QtEndian>

#include "wavpcm.h"

namespace {

QByteArray le16(quint16 v)
{
    QByteArray b(2, Qt::Uninitialized);
    qToLittleEndian<quint16>(v, b.data());
    return b;
}

QByteArray le32(quint32 v)
{
    QByteArray b(4, Qt::Uninitialized);
    qToLittleEndian<quint32>(v, b.data());
    return b;
}

QByteArray chunk(const char *id, const QByteArray &body)
{
    QByteArray c = QByteArray(id, 4) + le32(quint32(body.size())) + body;
    if (body.size() % 2)
        c.append('\0');             // 块按偶数字节对齐
    return c;
}

QByteArray fmtBody(quint16 format, quint16 channels, quint32 rate, quint16 bits)
{
    const quint16 blockAlign = quint16(channels * bits / 8);
    return le16(format) + le16(channels) + le32(rate)
         + le32(rate * blockAlign) + le16(blockAlign) + le16(bits);
}

QByteArray riff(const QByteArray &chunks)
{
    return QByteArray("RIFF") + le32(quint32(4 + chunks.size())) + QByteArray("WAVE") + chunks;
}

// 一个标准的 16 kHz / 单声道 / 16 位 PCM WAV。
QByteArray goodWav(const QByteArray &pcm)
{
    return riff(chunk("fmt ", fmtBody(1, 1, 16000, 16)) + chunk("data", pcm));
}

QByteArray samplePcm()
{
    return QByteArray::fromHex("0100feff0002fffd10277ff0");   // 6 个样本
}

} // namespace

class TestWavPcm : public QObject
{
    Q_OBJECT

private slots:
    // ---------- 接受 ----------

    void rawPcmPassesThroughUnchanged()
    {
        const QByteArray raw = samplePcm();
        QByteArray pcm;
        QString error;
        QVERIFY(WavPcm::extract(raw, &pcm, &error));
        QCOMPARE(pcm, raw);
        QVERIFY(error.isEmpty());
    }

    void headerIsStrippedFromValidWav()
    {
        // 这条就是原 bug：44 字节的文件头不能混进样本里。
        const QByteArray wav = goodWav(samplePcm());
        QCOMPARE(wav.size(), 44 + samplePcm().size());

        QByteArray pcm;
        QVERIFY(WavPcm::extract(wav, &pcm));
        QCOMPARE(pcm, samplePcm());
    }

    void unknownChunksBeforeDataAreSkipped()
    {
        // 奇数长度的 LIST 块后面有一个填充字节，跳错一个字节就找不到 data。
        const QByteArray wav = riff(chunk("fmt ", fmtBody(1, 1, 16000, 16))
                                    + chunk("LIST", QByteArray("INFOabc"))
                                    + chunk("data", samplePcm()));
        QByteArray pcm;
        QVERIFY(WavPcm::extract(wav, &pcm));
        QCOMPARE(pcm, samplePcm());
    }

    void chunksAfterDataAreNotIncluded()
    {
        const QByteArray wav = riff(chunk("fmt ", fmtBody(1, 1, 16000, 16))
                                    + chunk("data", samplePcm())
                                    + chunk("LIST", QByteArray("INFOtail")));
        QByteArray pcm;
        QVERIFY(WavPcm::extract(wav, &pcm));
        QCOMPARE(pcm, samplePcm());
    }

    void extensibleFormatWithPcmSubformatIsAccepted()
    {
        // WAVE_FORMAT_EXTENSIBLE：基本 16 字节 + cbSize(22) + 有效位数 + 声道掩码 + GUID
        QByteArray body = fmtBody(0xFFFE, 1, 16000, 16);
        body += le16(22) + le16(16) + le32(4);
        body += le16(1) + QByteArray(14, '\0');      // SubFormat，头两个字节 = PCM
        QCOMPARE(body.size(), 40);

        const QByteArray wav = riff(chunk("fmt ", body) + chunk("data", samplePcm()));
        QByteArray pcm;
        QVERIFY(WavPcm::extract(wav, &pcm));
        QCOMPARE(pcm, samplePcm());
    }

    void streamingWavWithUnknownDataLengthUsesRestOfFile_data()
    {
        QTest::addColumn<quint32>("declared");
        QTest::newRow("zero")      << quint32(0);
        QTest::newRow("all-ones")  << quint32(0xFFFFFFFFu);
        QTest::newRow("too-large") << quint32(1000000);   // 文件被截断
    }

    void streamingWavWithUnknownDataLengthUsesRestOfFile()
    {
        QFETCH(quint32, declared);
        const QByteArray wav = riff(chunk("fmt ", fmtBody(1, 1, 16000, 16))
                                    + QByteArray("data") + le32(declared) + samplePcm());
        QByteArray pcm;
        QVERIFY(WavPcm::extract(wav, &pcm));
        QCOMPARE(pcm, samplePcm());
    }

    void trailingHalfSampleIsDropped()
    {
        const QByteArray wav = riff(chunk("fmt ", fmtBody(1, 1, 16000, 16))
                                    + QByteArray("data") + le32(0) + samplePcm() + QByteArray("\x7f"));
        QByteArray pcm;
        QVERIFY(WavPcm::extract(wav, &pcm));
        QCOMPARE(pcm, samplePcm());
    }

    // ---------- 拒绝 ----------

    void wrongFormatIsRejectedWithActualFormatInMessage_data()
    {
        QTest::addColumn<quint16>("format");
        QTest::addColumn<quint16>("channels");
        QTest::addColumn<quint32>("rate");
        QTest::addColumn<quint16>("bits");
        QTest::addColumn<QString>("expectedInMessage");

        QTest::newRow("44.1 kHz")  << quint16(1) << quint16(1) << quint32(44100) << quint16(16) << "44100 Hz";
        QTest::newRow("8 kHz")     << quint16(1) << quint16(1) << quint32(8000)  << quint16(16) << "8000 Hz";
        QTest::newRow("stereo")    << quint16(1) << quint16(2) << quint32(16000) << quint16(16) << "2 声道";
        QTest::newRow("8-bit")     << quint16(1) << quint16(1) << quint32(16000) << quint16(8)  << "8 位";
        QTest::newRow("ieee-float")<< quint16(3) << quint16(1) << quint32(16000) << quint16(16) << "非 PCM";
    }

    void wrongFormatIsRejectedWithActualFormatInMessage()
    {
        QFETCH(quint16, format);
        QFETCH(quint16, channels);
        QFETCH(quint32, rate);
        QFETCH(quint16, bits);
        QFETCH(QString, expectedInMessage);

        const QByteArray wav = riff(chunk("fmt ", fmtBody(format, channels, rate, bits))
                                    + chunk("data", samplePcm()));
        QByteArray pcm = "stale";
        QString error;
        QVERIFY(!WavPcm::extract(wav, &pcm, &error));
        QVERIFY(pcm.isEmpty());          // 失败时不留下可被误用的数据
        QVERIFY2(error.contains(expectedInMessage), qPrintable(error));
    }

    void wavWithoutDataChunkIsRejected()
    {
        const QByteArray wav = riff(chunk("fmt ", fmtBody(1, 1, 16000, 16)));
        QByteArray pcm;
        QString error;
        QVERIFY(!WavPcm::extract(wav, &pcm, &error));
        QVERIFY2(error.contains("data"), qPrintable(error));
    }

    void wavWithDataBeforeFmtIsRejected()
    {
        // 没见到 fmt 就无从确认格式，不能把 data 当成合格的 PCM 交出去。
        const QByteArray wav = riff(chunk("data", samplePcm())
                                    + chunk("fmt ", fmtBody(1, 1, 16000, 16)));
        QByteArray pcm;
        QString error;
        QVERIFY(!WavPcm::extract(wav, &pcm, &error));
        QVERIFY2(error.contains("fmt"), qPrintable(error));
    }

    void truncatedFmtChunkIsRejected()
    {
        const QByteArray wav = riff(QByteArray("fmt ") + le32(16) + QByteArray(6, '\0'));
        QByteArray pcm;
        QString error;
        QVERIFY(!WavPcm::extract(wav, &pcm, &error));
        QVERIFY2(error.contains("fmt"), qPrintable(error));
    }

    void errorPointerIsOptional()
    {
        const QByteArray wav = riff(chunk("fmt ", fmtBody(1, 2, 16000, 16))
                                    + chunk("data", samplePcm()));
        QByteArray pcm;
        QVERIFY(!WavPcm::extract(wav, &pcm));
    }
};

QTEST_APPLESS_MAIN(TestWavPcm)
#include "tst_wavpcm.moc"
