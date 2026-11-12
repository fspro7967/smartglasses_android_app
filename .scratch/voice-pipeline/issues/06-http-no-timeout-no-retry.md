# 06 — AI 请求没有超时也没有重试,挂住的请求会永久占住界面状态

Status: resolved
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

## Answer

**2026-11-11 — 超时与错误分类已改完,并加了发送前的配置校验(代码在工作区,未经真机验证)。重试与并发策略没有做。**

- **超时**:`QNetworkRequest::setTransferTimeout()`。云端 API 15 秒(`kApiTimeout`),
  本地 Ollama **60 秒**(`kOllamaTimeout`)。两者不同是有意的:请求是非流式的,回复生成完
  之前服务器一个字节都不发,所以这个值同时也是「最长允许思考多久」;Ollama 首个请求要先
  把模型载入内存,冷启动几十秒很常见,按 15 秒掐会让每次开机后的第一句话都被误判为超时。
  **这与本工单判据 1 的「15 秒内」对 Ollama 这条路不一致**,如不认可请把 `kOllamaTimeout` 改回去。
- **错误分类**(`describeFailure()`):超时 / 服务端返回错误(带 HTTP 状态码和服务端给的原因)/
  连不上服务器,三类措辞不同。服务端原因只取已知结构里的那一个字段
  (`error.message` 或 `error`),截断到 200 字,不把整段正文原样上屏。
- **配置校验**:地址、模型(API 还有密钥)为空,或地址不是 http/https 时,直接报
  「尚未配置…」而不是拿空地址发一个注定失败的请求。地址结尾多余的斜杠会被去掉。
- 两条路共用的 system prompt 提成了常量 `kSystemPrompt`。

没有做的:建议做法 3(重试)和 4(并发/乱序策略)。后者与 `issues/04` 是同一个决定,
留在那边。真机判据转入 `issues/11`。
