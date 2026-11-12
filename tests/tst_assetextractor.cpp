// AssetExtractor 的单元测试。
//
// 模块只做文件复制与清单校验（QtCore），所以把一个普通临时目录当作「assets 目录」
// 就能在构建机上验证全部判定逻辑，不需要 APK。Android 上 `assets:/` 的差别只在
// 文件引擎，QFile / QFileInfo 的用法相同。
//
// 重点是 `.scratch/offline-tts` 工单 08 的场景：换了一份 APK（应用版本变了，或源
// 资产长度变了）之后，旧的落盘副本不能再被当成有效的。
//
// 运行方式（构建机上）：
//   cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug
//   cmake --build build-tests --target tst_assetextractor
//   ./build-tests/tests/tst_assetextractor

#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "assetextractor.h"

namespace {

void writeFile(const QString &path, const QByteArray &content)
{
    QFile f(path);
    QVERIFY2(f.open(QIODevice::WriteOnly | QIODevice::Truncate), qPrintable(path));
    QCOMPARE(f.write(content), qint64(content.size()));
}

QByteArray readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QByteArray("<unreadable>");
    return f.readAll();
}

const char kManifest[] = ".extract-manifest";

} // namespace

class TestAssetExtractor : public QObject
{
    Q_OBJECT

    QTemporaryDir m_tmp;
    QString m_assets;
    QString m_target;
    const QStringList m_names{QStringLiteral("model.onnx"), QStringLiteral("tokens.txt")};

private slots:
    void init()
    {
        // 每个用例一套全新的目录
        QVERIFY(m_tmp.isValid());
        static int counter = 0;
        const QString base = m_tmp.filePath(QString::number(++counter));
        m_assets = base + "/assets";
        m_target = base + "/target";
        QVERIFY(QDir().mkpath(m_assets));

        QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
        // 2.5 MiB：跨过 1 MiB 的分块边界，分块复制的拼接也一并验证
        writeFile(m_assets + "/model.onnx", QByteArray(2 * 1024 * 1024 + 512 * 1024, 'm'));
        writeFile(m_assets + "/tokens.txt", QByteArray("a 1\nb 2\n"));
    }

    // ---------- 基本保证 ----------

    void nothingExtractedInitially()
    {
        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target, m_names));
    }

    void extractCopiesEveryFileAndMarksComplete()
    {
        QString error;
        QVERIFY2(AssetExtractor::ensureExtracted(m_assets, m_target, m_names, &error),
                 qPrintable(error));
        for (const QString &name : m_names)
            QCOMPARE(readFile(m_target + '/' + name), readFile(m_assets + '/' + name));
        QVERIFY(AssetExtractor::isExtracted(m_assets, m_target, m_names));
        QVERIFY(!QFile::exists(m_target + "/model.onnx.tmp"));
    }

    void secondCallDoesNotCopyAgain()
    {
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        // 把落盘副本改成等长的别的内容：如果第二次又复制了，它会被源文件盖回去。
        writeFile(m_target + "/tokens.txt", QByteArray("X 1\nY 2\n"));
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        QCOMPARE(readFile(m_target + "/tokens.txt"), QByteArray("X 1\nY 2\n"));
    }

    void filesWithoutManifestAreNotTrusted()
    {
        // 进程在写清单之前被杀：文件都在，但没有「提取完整」的凭据。
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        QVERIFY(QFile::remove(m_target + '/' + kManifest));
        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target, m_names));
    }

    void truncatedCopyIsDetected()
    {
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        writeFile(m_target + "/model.onnx", QByteArray(1000, 'm'));
        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target, m_names));
    }

    void differentFileSetIsNotComplete()
    {
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target,
                                             {QStringLiteral("model.onnx")}));
    }

    void missingSourceFailsWithoutWritingManifest()
    {
        QStringList names = m_names;
        names << QStringLiteral("does-not-exist.fst");
        QString error;
        QVERIFY(!AssetExtractor::ensureExtracted(m_assets, m_target, names, &error));
        QVERIFY2(error.contains("does-not-exist.fst"), qPrintable(error));
        QVERIFY(!QFile::exists(m_target + '/' + kManifest));
    }

    // ---------- 过期检测：落盘副本必须来自「当前这份 APK」 ----------

    void changedSourceLengthTriggersReextraction()
    {
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));

        // 新 APK 带了一个长度不同的模型
        const QByteArray newModel(3 * 1024 * 1024, 'n');
        writeFile(m_assets + "/model.onnx", newModel);

        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target, m_names));
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        QCOMPARE(readFile(m_target + "/model.onnx"), newModel);
        QVERIFY(AssetExtractor::isExtracted(m_assets, m_target, m_names));
    }

    void changedAppVersionTriggersReextractionEvenWithSameLength()
    {
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));

        // 长度相同、内容不同的模型：只看长度是看不出来的，靠版本号兜住。
        const QByteArray sameLength("c 3\nd 4\n");
        QCOMPARE(sameLength.size(), readFile(m_assets + "/tokens.txt").size());
        writeFile(m_assets + "/tokens.txt", sameLength);
        QVERIFY(AssetExtractor::isExtracted(m_assets, m_target, m_names));   // 版本没变：看不出来

        QCoreApplication::setApplicationVersion(QStringLiteral("1.1"));
        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target, m_names));
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        QCOMPARE(readFile(m_target + "/tokens.txt"), sameLength);
        QVERIFY(AssetExtractor::isExtracted(m_assets, m_target, m_names));
    }

    void oldFormatManifestCausesOneReextractionNotARetryLoop()
    {
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));

        // 旧版本写的清单没有首行的格式/版本标记，直接就是「文件名\t长度」。
        QByteArray v1;
        for (const QString &name : m_names) {
            v1 += name.toUtf8() + '\t'
                + QByteArray::number(QFileInfo(m_target + '/' + name).size()) + '\n';
        }
        writeFile(m_target + '/' + kManifest, v1);

        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target, m_names));
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        // 重提之后写出的是新格式，下一次启动不会再重提
        QVERIFY(AssetExtractor::isExtracted(m_assets, m_target, m_names));
        QVERIFY(readFile(m_target + '/' + kManifest).startsWith("#extract-manifest-v2\t1.0\n"));
    }

    void garbageManifestIsNotComplete()
    {
        QVERIFY(AssetExtractor::ensureExtracted(m_assets, m_target, m_names));
        writeFile(m_target + '/' + kManifest,
                  QByteArray("#extract-manifest-v2\t1.0\nmodel.onnx\tnot-a-number\n"));
        QVERIFY(!AssetExtractor::isExtracted(m_assets, m_target, m_names));
    }
};

QTEST_GUILESS_MAIN(TestAssetExtractor)
#include "tst_assetextractor.moc"
