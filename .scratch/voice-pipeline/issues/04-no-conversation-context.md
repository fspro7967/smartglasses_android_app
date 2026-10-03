# 04 — 没有会话/上下文概念:每个 3 秒音频块都是一次独立的单轮问答

Status: needs-triage
Type: task

## 现状

`MainWindow::onTranscriptionResult()`(`src/mainwindow.cpp:624-635`)拿到一段转写就:

```cpp
m_msgsender->sendMessage(text);
```

而 `MsgSender` 每次请求都**新建**一个只有两条消息的 payload
(`src/msgsender.cpp:86-97` 与 `:149-160`,两条路径相同):

```cpp
systemMsg = "你是智能眼镜上的语音助手，回答请简洁、口语化。";
userMsg   = message;
payload["messages"] = QJsonArray{systemMsg, userMsg};
```

没有任何历史、会话 id 或轮次状态。于是:

- 使用者说「帮我查一下明天的天气」→ 得到回答;接着说「那后天呢?」→
  模型看到的是一句孤立的「那后天呢?」,不知道在问什么。
- 每 3 秒的音频块各触发一次请求;一句话被切成多块时会得到多次互相不知道对方存在的回答。
- 前一次请求还在路上时又发起下一次,回复顺序不保证 —— 界面上后到的短回复可能覆盖先到的长回复。

## 这是个缺陷还是特性?需要所有者定夺

可能的方向:

1. **加会话历史**:`MsgSender` 维护一个滚动的消息列表(带长度上限),
   `sendMessage()` 带上它;并提供「新会话」入口。
2. **改成轮次式**:不再按固定 3 秒切,而是靠静音检测确定「一句话说完了」
   (与 `.scratch/voice-pipeline/issues/02` 的冲刷定时器是同一件事),
   一轮对应一次请求,天然消除「一句话多次提问」。
3. **明确不做**:如果目标只是「一问一答的短指令」,那就把当前行为写成规格,
   并顺手解决「回复乱序覆盖」这一点(至少按请求序号丢弃过期回复)。

## 相关

- `MsgSender` 的配置持久化见 `src/msgsender.cpp:26-33`
- 请求没有超时、没有重试:`issues/06`
- 离线场景(无网络时完全没有回复):`issues/07`
