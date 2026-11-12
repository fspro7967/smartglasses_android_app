# 工单总览

本目录是这个仓库的工单系统:每个专题一个目录,规格在 `<专题>/spec.md`,工单一张一个文件在
`<专题>/issues/NN-<slug>.md`。约定见 `docs/agents/issue-tracker.md`,状态用词见
`docs/agents/triage-labels.md`。

**每张工单的状态以它自己文件里的 `Status:` 行为准**,下面的表只是目录——改了工单请顺手改这里。
最近一次整体核对:2026-11-11。

## 专题

| 目录 | 内容 | 规格的性质 |
|---|---|---|
| [`offline-tts/`](offline-tts/spec.md) | 离线中文语音合成与手机端朗读(sherpa-onnx) | 冻结规格,已实施,待真机验收 |
| [`ble-audio-out/`](ble-audio-out/spec.md) | TTS 音频经 BLE 回传眼镜(下行队列、背压) | 冻结规格,已实施,待真机验收 |
| [`board-esp32-link/`](board-esp32-link/spec.md) | ESP32-C3 开发板接入(自动配置、控制通道、固件) | 盘点记录 |
| [`voice-pipeline/`](voice-pipeline/spec.md) | 上行音频 → 识别 → 应答 → 朗读排队 | 盘点记录 |
| [`repo-hygiene/`](repo-hygiene/spec.md) | 构建、密钥、测试、配置 | 盘点记录 |

调研材料(TTS 引擎选型的六份文档)在 `offline-tts/research/`;`offline-tts/handoff.md` 是
2026-09 那次交接的原始文档,作为历史记录保留。

## 等所有者决定(needs-triage)

| 工单 | 要决定的事 |
|---|---|
| `voice-pipeline/issues/03` | 被「暂时」注释掉的 whisper 解码参数:恢复、删除,还是把理由写清楚 |
| `voice-pipeline/issues/04` | 要不要对话历史 / 按一句话而不是按 3 秒切段 |
| `voice-pipeline/issues/07` | 断网时的应答能力做不做 |
| `board-esp32-link/issues/08` | 固件的 volume / status / echo 指令要不要在手机侧用起来 |
| `repo-hygiene/issues/03` | 测试、CI、`file(GLOB)` 做到哪一步 |
| `repo-hygiene/issues/04` | 这个应用要不要多语言 |

## 可以直接动手(ready-for-agent)

| 工单 | 内容 |
|---|---|
| `repo-hygiene/issues/02` | 给 OpenSSL 预编译库补获取脚本与缺失检查(全新克隆目前无法构建) |

## 等真机(ready-for-human)

这些都只能在设备上判;代码侧已经做完。

| 工单 | 验什么 |
|---|---|
| `offline-tts/issues/02` | TTS 模型提取:首次出现、二次跳过、杀进程不留半截、换模型后重新提取 |
| `offline-tts/issues/03` | whisper 模型不再落盘,转写仍工作 |
| `offline-tts/issues/04` | 能听到声音,「停止」立即生效 |
| `offline-tts/issues/05` | 端到端:说英文 → 中文回复被念出来 |
| `ble-audio-out/issues/04` | 回传出声、手机静音、不溢出、停止跟手、写入错误后不卡死 |
| `board-esp32-link/issues/05` | 开发板验收:上下行、自动配置、play 确认、断开重连、固件两处修正 |
| `voice-pipeline/issues/11` | 2026-11-11 这批语音管线修改的验收 |

## 全部工单

### offline-tts

| # | 标题 | 状态 |
|---|---|---|
| 01 | 确认 Qt 6.11.1 的 Android 套件含 Multimedia 模块 | resolved |
| 02 | 验证 TTS 模型提取 | ready-for-human |
| 03 | 验证 AppDataLocation 下不再出现 whisper 模型 | ready-for-human |
| 04 | 验证能听到声音,且「停止」立即生效 | ready-for-human |
| 05 | 端到端:说一句英文,得到中文回复并被朗读 | ready-for-human |
| 06 | 在构建机上跑 tst_sentencesplitter | resolved |
| 07 | 记录真实的 APK 体积增量 | resolved |
| 08 | 提取清单只校验落盘副本自身 | resolved |

### ble-audio-out

| # | 标题 | 状态 |
|---|---|---|
| 01 | DeviceHandler 支持流式下行 | resolved |
| 02 | MainWindow 把合成音频接到 BLE 下行 | resolved |
| 03 | QML 音频回传开关 | resolved |
| 04 | 真机上验证音频回传 | ready-for-human |
| 05 | 有响应写入失败一次后,下行永久卡死 | resolved |

### board-esp32-link

| # | 标题 | 状态 |
|---|---|---|
| 01 | 开发板适配改动未提交、无规格、无工单 | resolved |
| 02 | 固件未切换 MTU + 播放特征只支持有响应写 | resolved |
| 03 | 自动配置宣告成功但从不校验 | resolved |
| 04 | 控制指令写入不记账,`play` 无确认 | resolved |
| 05 | 开发板真机验收 | ready-for-human |
| 06 | 设备意外断开后 DeviceHandler 不清理自身状态 | resolved |
| 07 | 固件:断开时不复位播放态;去直流滤波无饱和 | resolved |
| 08 | 固件的 volume / status / echo 指令手机侧没有使用 | needs-triage |

### voice-pipeline

| # | 标题 | 状态 |
|---|---|---|
| 01 | 音频缓冲被局部变量遮蔽;忙时整块丢弃 | resolved |
| 02 | 尾部音频永不转写;断开时缓冲不清 | resolved |
| 03 | 被「暂时」注释掉的 whisper 解码参数 | needs-triage |
| 04 | 没有会话/上下文概念 | needs-triage |
| 05 | Whisper 初始化挡住首屏 | resolved |
| 06 | AI 请求没有超时也没有重试 | resolved |
| 07 | 断网时没有任何回复能力 | needs-triage |
| 08 | 朗读途中来了新回复,朗读队列被覆盖 | resolved |
| 09 | 朗读期间上行音频仍进入识别 | wontfix |
| 10 | 「选音频」把 WAV 文件头当 PCM | resolved |
| 11 | 语音管线真机验收 | ready-for-human |

### repo-hygiene

| # | 标题 | 状态 |
|---|---|---|
| 01 | 签名密钥与口令被跟踪进仓库 | resolved |
| 02 | OpenSSL 预编译库没入库也没有拉取脚本 | ready-for-agent |
| 03 | 测试少、没有 CI、CMake 用 file(GLOB) | needs-triage |
| 04 | 翻译目录是空的 | needs-triage |
| 05 | CMake 的 OpenMP 开关与优化选项没有生效 | resolved |
| 06 | API Key 明文 / 可备份 / 明文流量 / 示例包名 | wontfix |
| 07 | .vscode 配置残缺 | wontfix |
| 08 | CMake 杂项 | wontfix |
| 09 | 模型与第三方源码直接入库 | wontfix |
| 10 | README 的排版与措辞 | wontfix |

## 关于「resolved」

2026-11-11 关闭的那批工单(`voice-pipeline` 01/02/05/06/08/10、`board-esp32-link` 03/04/06/07、
`ble-audio-out` 05、`offline-tts` 08、`repo-hygiene` 05)指的是**代码已改完并通过构建**
(Android Release 目标、固件两个环境、3 组单元测试)。凡是只能在设备上确认的判据,都已转到上面
「等真机」里对应的验收工单,没有因为工单关闭而消失。
