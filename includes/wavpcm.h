#ifndef WAVPCM_H
#define WAVPCM_H

#include <QByteArray>
#include <QString>

// 从一个音频文件的全部内容里取出可直接送进 Whisper 的 PCM 数据区。
//
// 纯自由函数：只依赖 QtCore，可以在没有 Android 设备的情况下单元测试
// （见 tests/tst_wavpcm.cpp）。
//
// Whisper 只吃 16 kHz 单声道 int16 小端。以前「选音频」把整个文件——包括
// WAV 的 44 字节文件头——当作 PCM 念进去，采样率和声道数也不看：头部变成一声
// 爆音，44.1 kHz 或立体声的文件则被当成 16 kHz 单声道，识别结果是乱码且没有
// 任何提示。这里把格式核实清楚，不符合就明确拒绝并说出原因。
namespace WavPcm {

// Whisper 要求的输入格式。
inline constexpr int kSampleRate    = 16000;
inline constexpr int kChannels      = 1;
inline constexpr int kBitsPerSample = 16;

// fileData 以 "RIFF....WAVE" 开头时按 WAV 解析：逐块走到 "fmt " 核对格式，
// 再取 "data" 块的内容；格式不是 16 kHz / 单声道 / 16 位 PCM 时返回 false，
// errorOut 写明文件的实际格式。
//
// 不是 RIFF/WAVE 的文件（例如 .pcm）视为裸 PCM 原样返回——裸数据没有任何可供
// 校验的信息，格式由使用者保证。
bool extract(const QByteArray &fileData, QByteArray *pcmOut, QString *errorOut = nullptr);

} // namespace WavPcm

#endif // WAVPCM_H
