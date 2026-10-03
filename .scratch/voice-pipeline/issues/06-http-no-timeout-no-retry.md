# 06 — AI 请求没有超时也没有重试,挂住的请求会永久占住界面状态

Status: ready-for-agent
Type: bug

## 现状

`MsgSender::sendToApi()` / `sendToOllama()`(`src/msgsender.cpp:80-107,144-170`)各自
`m_networkManager->post(...)`,然后只等 `QNetworkReply::finished`。
`QNetworkRequest` 上**没有** `setTransferTimeout()`,也没有自己的 `QTimer` 兜底。

后果:

- 服务器接受连接但不回包(TCP 半开、代理吞包、局域网 IP 填错时的某些路由),
  `finished` 可能长时间不发出;界面既没有「失败」也没有「成功」,使用者只会以为 App 卡了。
- `handleApiReply()` / `handleOllamaReply()` 只在 `reply->error()` 上判定
  (`src/msgsender.cpp:113-115,176-178`)。HTTP 401/429/500 这类**有响应体**的错误
  在 Qt 里也会置 `error()`,但错误文本只有通用描述,拿不到服务端返回的 message,
  排查时要靠抓包。建议在 `error()` 分支里把 `reply->readAll()` 的正文一起带出来
  (注意 401 时正文可能含敏感字段,取舍由所有者定)。
- 没有重试。移动网络下一次瞬时失败就是一次无回复。

## 建议做法

1. 每个请求设 `request.setTransferTimeout(15'000)`(Qt 6 起支持),
   或在 `MsgSender` 里用 `QTimer` 跟踪在途请求。
2. 超时时 `reply->abort()`,走既有的 `errorOccurred` 路径
   —— 它已经接到只显示不朗读的静默策略上(`src/mainwindow.cpp:231-234`),不需要新接线。
3. (可选)对幂等的 chat 请求做一次重试,指数退避;重试前必须确认前一个 reply 已丢弃。
4. 在途请求的并发策略:当前允许无限并发(见 `issues/04` 的乱序问题),
   至少给「同一时刻只保留最新一个请求」加个显式决定。

## 完成判据

- [ ] 把服务器地址指向一个不回包的端点,15 秒内界面出现明确错误
- [ ] 错误信息能区分「连不上」「超时」「服务端返回错误」

## 相关

- 配置持久化与 key 明文:`src/msgsender.cpp:26-33`
- 会话/乱序:`issues/04`
