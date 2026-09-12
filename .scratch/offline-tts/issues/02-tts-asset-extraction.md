# 02 — 验证 TTS 模型提取：首次出现、二次跳过、杀进程不留半截文件

Status: ready-for-human
Type: task

对应 spec.md 步骤 2 的完成判据 [构建机]：

> 首次启动后目标文件出现；第二次启动不重复复制；
> **手工杀掉进程再启动，不会留下被误判为有效的半截文件**。

关注点：`AssetExtractor` 把「提取完整」的凭据放在 `.extract-manifest`，而该文件
是在所有目标文件都 rename 成功之后才写的。第 3 条判据要验证的正是这一点——
中途 kill 之后，目标目录里不应存在一个长度正确、但内容不完整的文件。

怎么测第 3 条：启动应用，在复制进行中 `adb shell am force-stop <包名>`（或更早地
kill -9），然后重启，检查 `AppDataLocation/tts-vits-icefall-zh-aishell3/` 下
是否残留 `.tmp` 文件、以及应用是否重新完整提取而不是直接认为已就绪。

目标目录形如 `/data/data/<包名>/files/tts-vits-icefall-zh-aishell3/`。

## Comments

**2026-09-12（构建机）** — 未做，仍需要真机。构建机侧能确认的前置条件已满足：
`AssetExtractor::ensureExtracted()` 的四个要求（后台线程、1 MiB 分块、
`.tmp` + rename、清单按字节长度校验）在代码中逐条可读，且 8 个模型文件确实
随 APK 打包到 `assets/models/tts-vits-icefall-zh-aishell3/`（见工单 07）。
判据本身（首次出现 / 二次跳过 / 杀进程不留半截）必须在设备上跑。
