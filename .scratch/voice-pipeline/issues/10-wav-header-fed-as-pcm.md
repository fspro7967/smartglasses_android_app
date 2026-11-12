# 10 — 「选音频」把 WAV 文件头当 PCM,也不校验采样率与声道

Status: resolved
Type: bug

## 问题

`WhisperManager::processAudioFile()` 原来把文件内容整个 `reinterpret_cast` 成 int16 样本。
文件对话框的过滤器是 `*.wav *.pcm`,而 WAV 有 44 字节(或更长)的文件头:

- 文件头被当作样本念进去,开头是一小段噪声;
- 采样率、声道数、位深一概不看。44.1 kHz 或立体声的文件被当成 16 kHz 单声道,
  识别结果是乱码,而界面照常显示「已将音频文件送入模型识别」,没有任何提示。

2026-11-11 通读代码时发现,此前没有工单。

## Answer

**2026-11-11 — 已修,带单元测试。**

- 新增纯函数模块 `includes/wavpcm.h` + `src/wavpcm.cpp`:`WavPcm::extract()`。
  以 `RIFF…WAVE` 开头的按 WAV 逐块解析,核对 `fmt ` 块必须是 16 kHz / 单声道 / 16 位 PCM
  (含 `WAVE_FORMAT_EXTENSIBLE` 的 PCM 子格式),再取 `data` 块;不符合就拒绝,错误信息里写出
  文件的实际格式。不是 RIFF/WAVE 的文件(`.pcm`)视为裸 PCM 原样使用——裸数据没有可校验的
  信息,格式仍由使用者保证。
- 容错:`data` 长度为 0 / 0xFFFFFFFF(流式写出的 WAV)或声明得比文件长(被截断)时,
  以文件里真实剩下的字节为准;末尾半个样本丢弃;`data` 之前的未知块按偶数对齐跳过。
- `processAudioFile()` 返回 `bool`,`MainWindow` 只在真的受理之后才显示「已将音频文件送入
  模型识别」;模型未就绪或识别队列已满时明确拒绝。

验证:`tests/tst_wavpcm.cpp`(13 个测试函数,按数据行展开 18 项)在本机 host Qt 上全部通过;
Android Release 目标编译链接通过。真机上点一次「选音频」的确认并入 `issues/11`。
