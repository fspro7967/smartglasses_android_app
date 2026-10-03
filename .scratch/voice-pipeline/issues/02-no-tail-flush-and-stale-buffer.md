# 02 — 尾部音频永不转写;断开时缓冲不清,跨会话残留

Status: ready-for-agent
Type: bug

## 问题

`MainWindow::onDataReceived()`(`src/mainwindow.cpp:584-599`)只按**固定字节数**触发转写:

```cpp
constexpr int CHUNK_SIZE_BYTES = 3 * 16000 * sizeof(int16_t);   // 96000 B ≈ 3 秒
m_audioBuffer.append(data);
if (m_audioBuffer.size() >= CHUNK_SIZE_BYTES && m_whisperManager) {
    ... feedAudioData(...);
    m_audioBuffer.clear();
}
```

没有第二条件。后果:

1. **最后不足 3 秒的语音永远不会被识别。** 使用者说完一句就停下、或蓝牙静默之后,
   残留在 `m_audioBuffer` 里的 0–95999 字节没有任何路径送进 Whisper。
   对「短指令」型的用法,丢的经常正是全部内容。
2. **断开连接不清缓冲。** `disconnected` 槽(`src/mainwindow.cpp:172-179`)只清服务与
   写入目标;`m_audioBuffer` 保持原样。下次连接(可能是另一台设备)继续 append,
   于是两次会话的音频被拼进同一个 3 秒窗口,识别结果跨会话串味。
3. `WhisperManager::reset()`(`includes/whisper_manager.h:30`)在整个仓库**没有调用方**
   (`grep -rn "reset()"` 只匹配到 sink 的 `m_audioSink->reset()`),
   是一个为「清空输入」而写、却没人用的接口。
4. 固定 3 秒窗口也没有静音/语音活动检测:说话与静音被一起切,Whisper 容易在静音段
   产生幻觉文本(这正是 `whisper_manager.cpp:121-129` 那批参数想压的东西)。

## 建议做法

1. **加冲刷定时器**:每收到数据就重启一个 300–800 ms 的单次定时器;超时且缓冲达到一个
   最小长度(例如 0.5 秒,避免把噪声当语音)时,把当前缓冲交给 Whisper 并清空。
   这与既有的 `m_audioPump` 模式一致,都是「不能只靠新数据驱动」。
2. **断开/切换设备时清缓冲**:在 `disconnected` 与 `connectToDevice` 两处调用
   `WhisperManager::reset()` 并 `m_audioBuffer.clear()`。
3. 把「多少字节触发」与「多久没数据触发」两个条件都写进注释,避免后人只看到一边。

## 完成判据

- [ ] 说一句 1 秒的短指令,松口后 1 秒内出现识别结果(无需再凑满 3 秒)
- [ ] 断开后重连,上一段的残留音频不会出现在新的识别结果里
- [ ] `WhisperManager::reset()` 有真实调用方,或删除

## 相关

- 上游丢音频:`.scratch/voice-pipeline/issues/01`
- 设备端每包 64 样本(128 B)、约 250 包/秒:`smartglasses_board/src/main.cpp:18,466-485`
