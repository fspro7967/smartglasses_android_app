# 05 — Whisper 初始化仍在构造函数里同步跑,首屏被 57 MB 模型解析挡住

Status: ready-for-agent
Type: task

## 现状

`MainWindow` 构造函数(`src/mainwindow.cpp:133-237`)在最后几行同步调用:

```cpp
if (!m_whisperManager->init(QStringLiteral("assets:/models/model.bin"))) { ... }
```

`WhisperManager::init()`(`src/whisper_manager.cpp:24-53`)会

1. `QByteArray modelData = modelFile.readAll()` —— 把 59,707,625 B 的模型**整块**读进内存,
2. 调 `whisper_init_from_buffer_with_params()` 同步解析全部权重。

这两步都在 GUI 线程、在首屏出现之前完成。

TTS 那条路已经按 `.scratch/offline-tts/spec.md` 步骤 2 的硬要求放进了
`QtConcurrent` 后台线程(`MainWindow::prepareTtsModel()`,`src/mainwindow.cpp:688-724`),
whisper 没有。规格的「已知风险」表里也记了这一条:

> whisper 初始化仍在构造函数里 …… 规格未要求改，故未改

现在把它变成一张工单。

## 建议做法

照 `prepareTtsModel()` 的既有模式改,不要新造一套:

1. 构造函数里只接线,`init()` 丢进 `QtConcurrent::run`;
2. 结果用信号投递回 GUI 线程(`ttsPrepared` 的对应物);
3. 增加一个 `whisperReady` 属性,`onDataReceived()` / `processAudioFile()` 在未就绪时
   明确拒绝并提示,而不是继续喂数据(这与 `SpeechSynthesizer::isReady()` 的纪律一致,
   见 `includes/speechsynthesizer.h:19-29` 里对 `WhisperManager` 的批评);
4. 析构时 `waitForFinished()`,与 `m_ttsFutures` 的处理相同
   (`src/mainwindow.cpp:239-246`)。

顺带:当前 `init()` 失败只在状态栏说一句「Whisper 模型加载失败」,
之后 `feedAudioData()` 仍会收数据并在 worker 里发现 `m_ctx == nullptr` 才报错
(`src/whisper_manager.cpp:92-96`)—— 第 3 点正好把这个也一起解决。

## 完成判据

- [ ] 冷启动首屏在模型加载完成前就能交互
- [ ] 模型未就绪时喂音频会被明确拒绝,而不是先收下再逐块报错
- [ ] 退出时不崩溃(后台任务被等待)

## 相关

- 对标实现:`src/mainwindow.cpp:688-724`(`prepareTtsModel`)
- 接口纪律说明:`includes/speechsynthesizer.h:19-29`
