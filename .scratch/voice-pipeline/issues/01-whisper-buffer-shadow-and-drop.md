# 01 — 音频缓冲被局部变量遮蔽;转写进行中到达的音频被整块丢弃

Status: ready-for-agent
Type: bug

## 问题一:局部变量遮蔽了成员

`WhisperManager::feedAudioData()`(`src/whisper_manager.cpp:55-88`)第一行是:

```cpp
void WhisperManager::feedAudioData(const int16_t *data, size_t sampleCount)
{
    std::vector<float> m_audioBuffer ;     // ← 局部变量,遮蔽了同名成员
```

成员 `m_audioBuffer`(`includes/whisper_manager.h:40`)在整个文件里**从未被读写**;
`reset()`(`src/whisper_manager.cpp:219-223`)清的也是那个死成员。于是:

- 标着 `m_` 前缀、看起来是对象状态的东西,实际上是每次调用即生即灭的栈上 vector;
- 若哪天有人直接用小分片调用 `feedAudioData()`(比如不再在 `MainWindow` 里凑 3 秒),
  这段代码**永远不会触发转写** —— 局部 buffer 每次都被丢掉,size 永远到不了 `CHUNK_SIZE`;
  现在只是因为调用方每次都喂 3 秒(48000 样本 > `CHUNK_SIZE` 16000)才没暴露。

## 问题二:正在转写时到达的音频被静默丢弃(这条是真丢语音)

```cpp
if (m_audioBuffer.size() >= CHUNK_SIZE && !m_isProcessing) {
    m_isProcessing = true;
    auto audioChunk = std::move(m_audioBuffer);
    QFuture<void> future = QtConcurrent::run(...);
    ...
}
// 函数返回,局部 m_audioBuffer 析构 —— 本次 data 全部消失
```

`m_isProcessing` 为 true 时,条件整体不成立,**本次传入的 3 秒音频直接蒸发**,
既不排队也不上报。`MainWindow::onDataReceived` 在调用 `feedAudioData()` 之后
无条件 `m_audioBuffer.clear()`(`src/mainwindow.cpp:597`),所以那段音频在应用侧也没有留下。

触发条件很现实:使用者连续说话,而上一段 3 秒的转写在下一段凑满时还没跑完。
Tiny/Base 模型在手机上转写 3 秒音频需要几百毫秒到数秒,重叠不难发生。
表现是「中间一整句没识别」,且没有任何日志说明它被丢了。

## 问题三:`m_isProcessing` 跨线程无同步

worker 线程写(`src/whisper_manager.cpp:141,167`),GUI 线程读(:71),
类型是普通 `bool`(`includes/whisper_manager.h:41`)。这是数据竞争(UB),
虽然实践中多半只是偶尔读到旧值,但不应靠运气。

## 建议做法

1. 删掉局部变量,让 `feedAudioData()` 真正使用成员 buffer ——
   或者明确删掉成员并改名为局部 `samples`,二者选一,不留「看起来像状态」的哑变量。
2. 用**有界待处理队列**代替「忙就丢」:
   - `m_pendingChunks`(每个元素一段 `std::vector<float>`),上限按内存设
     (例如 4 段 ≈ 12 秒);
   - `feedAudioData()` 只负责入队,worker 完成后取下一段;
   - 队列满时**必须上报**(状态栏或 `errorOccurred`),不能再无声丢弃。
3. `m_isProcessing` 改 `std::atomic_bool`,或把状态收进一个只由 GUI 线程改、
   worker 只读的原子。
4. 现有「凑满 CHUNK_SIZE 才转写」的语义要么在成员 buffer 上真正实现,要么删掉 ——
   当前它在调用方已经凑过 3 秒的前提下是死逻辑。

## 完成判据

- [ ] 连续说 3 段话,3 段都有识别结果(或明确报告了丢弃,而不是静默丢)
- [ ] `grep -n "std::vector<float> m_audioBuffer" src/whisper_manager.cpp`
      不再出现「局部与成员同名」
- [ ] `m_isProcessing` 的读写有明确同步手段

## 相关

- 调用方:`src/mainwindow.cpp:584-599`
- 同批发现:`.scratch/voice-pipeline/issues/02`(尾部音频永不转写)
