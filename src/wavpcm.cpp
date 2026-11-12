#include "wavpcm.h"

#include <QtEndian>

namespace {

constexpr quint16 kFormatPcm        = 0x0001;
constexpr quint16 kFormatExtensible = 0xFFFE;   // 真实格式在扩展区的 SubFormat 里

void setError(QString *out, const QString &msg)
{
    if (out)
        *out = msg;
}

quint16 le16(const QByteArray &data, qsizetype offset)
{
    return qFromLittleEndian<quint16>(data.constData() + offset);
}

quint32 le32(const QByteArray &data, qsizetype offset)
{
    return qFromLittleEndian<quint32>(data.constData() + offset);
}

} // namespace

namespace WavPcm {

bool extract(const QByteArray &fileData, QByteArray *pcmOut, QString *errorOut)
{
    if (!pcmOut) {
        setError(errorOut, QStringLiteral("内部错误：未提供输出缓冲"));
        return false;
    }
    pcmOut->clear();

    const bool isWav = fileData.size() >= 12
                       && fileData.startsWith("RIFF")
                       && fileData.mid(8, 4) == "WAVE";
    if (!isWav) {
        // 裸 PCM：没有头可校验，原样交出去。
        *pcmOut = fileData;
        return true;
    }

    bool haveFormat = false;
    qsizetype pos = 12;   // 跳过 "RIFF" + 总长度 + "WAVE"
    while (pos + 8 <= fileData.size()) {
        const QByteArray id = fileData.mid(pos, 4);
        const quint32 size  = le32(fileData, pos + 4);
        const qsizetype body = pos + 8;

        if (id == "fmt ") {
            if (size < 16 || body + 16 > fileData.size()) {
                setError(errorOut, QStringLiteral("WAV 文件的 fmt 块不完整"));
                return false;
            }
            quint16 format        = le16(fileData, body);
            const quint16 channels = le16(fileData, body + 2);
            const quint32 rate     = le32(fileData, body + 4);
            const quint16 bits     = le16(fileData, body + 14);

            // WAVE_FORMAT_EXTENSIBLE：真实编码是 SubFormat GUID 的头两个字节。
            if (format == kFormatExtensible && size >= 40 && body + 26 <= fileData.size())
                format = le16(fileData, body + 24);

            if (format != kFormatPcm || channels != kChannels
                    || rate != quint32(kSampleRate) || bits != kBitsPerSample) {
                setError(errorOut,
                         QStringLiteral("仅支持 16 kHz 单声道 16 位 PCM 的 WAV，"
                                        "该文件为 %1 Hz / %2 声道 / %3 位%4")
                             .arg(rate).arg(channels).arg(bits)
                             .arg(format == kFormatPcm ? QString()
                                                       : QStringLiteral(" / 非 PCM 编码")));
                return false;
            }
            haveFormat = true;
        } else if (id == "data") {
            if (!haveFormat) {
                setError(errorOut, QStringLiteral("WAV 文件缺少 fmt 块，无法确认音频格式"));
                return false;
            }
            // 流式写出的 WAV 常把 data 长度留成 0 或 0xFFFFFFFF，被截断的文件则会
            // 声明得比实际长：一律以文件里真实剩下的字节数为准。
            const qsizetype available = fileData.size() - body;
            qsizetype length = (size == 0 || size == 0xFFFFFFFFu || qsizetype(size) > available)
                                   ? available : qsizetype(size);
            length -= length % 2;   // 末尾半个样本没有意义
            *pcmOut = fileData.mid(body, length);
            return true;
        }

        // 块内容按偶数字节对齐，奇数长度的块后面有一个填充字节。
        pos = body + qsizetype(size) + qsizetype(size & 1u);
    }

    setError(errorOut, QStringLiteral("WAV 文件缺少 data 块"));
    return false;
}

} // namespace WavPcm
