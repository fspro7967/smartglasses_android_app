# 交接：离线中文 TTS（`feature/ai-text-to-speech`）

**给谁：** 有 C++ / Qt / Linux + Android 工具链的同事
**发起人：** 实施者（Windows 开发机，**无 Qt、无 CMake、无 C++ 编译器**）
**分支：** `feature/ai-text-to-speech`（已推送，commit `43952f3`）
**基线：** `master` = `5d21a03`（**未合并，请不要合并**）
**回退点：** 本地 tag `snapshot/pre-tts-implementation` → `5d21a03`

---

## 0. 一句话说明你要做什么

代码写完了但**一行都没有编译过**（实施机上没有工具链）。你的工作是**在构建机上把它跑起来**，
按第 4 节的 7 条判据逐条验收，并把结果回执给发起人。其中第 3 节的两个问题**会直接决定
代码是否需要修改**，请优先确认。

---

## 1. 拿到代码并准备依赖

```bash
git clone https://github.com/fspro7967/smartglasses_android_app.git
cd smartglasses_android_app
git checkout feature/ai-text-to-speech
```

### 必须先做的一步

sherpa-onnx 的预编译 `.so` **不在版本库里**（`.gitignore` 全局排除 `*.so`），必须拉取：

```bash
bash third_party/sherpa-onnx/download-libs.sh
```

脚本会下载 45.2 MB 的归档并**校验 sha256**，只取 `arm64-v8a` 的两个 `.so`：

| 文件 | 字节数 | sha256 前缀 |
|---|---:|---|
| `libsherpa-onnx-c-api.so` | 4,456,784 | `73dba26d` |
| `libonnxruntime.so` | 21,684,880 | `dc5e4c17` |

**没有这一步，CMake configure 会在 `sherpa-onnx.cmake` 里报明确的 `FATAL_ERROR`**
（这是刻意设计的，避免变成晦涩的链接错误）。脚本是幂等的，已就绪时直接跳过。

> 若你在国内且 `github.com` 直连超时：git/libcurl **不读** Windows 注册表里的系统代理，
> 需要显式指定，例如 `git -c http.proxy=http://127.0.0.1:7897 clone ...`（端口换成你自己的）。
> 脚本本身走 `curl`/`wget`，记得设 `https_proxy` 环境变量。

TTS 模型**已随仓库提交**，不需要额外下载：
`android/assets/models/tts-vits-icefall-zh-aishell3/`（8 个文件，31.24 MB）。

---

## 2. 你需要的工具链

按 `.vscode/settings.example.json` 配置自己的 `settings.json`：

| 项 | 值 |
|---|---|
| Qt | 6.11.1，`android_arm64_v8a` + host `gcc_64` |
| ABI | `arm64-v8a` |
| Android platform | `android-28` |
| NDK | 27.2.12479018 |
| JDK | Android Studio 自带 jbr |
| host Qt 需含 **Qt6Test**（跑单元测试用） | |

**本机原始 `settings.json` 里含明文签名口令，请不要照抄**——见第 5 节「签名密钥」。

---

## 3. 两个「会决定代码要不要改」的问题 —— 请优先确认

### 3.1 Qt Multimedia 模块是否存在

`CMakeLists.txt` 现在 `find_package(Qt6 REQUIRED COMPONENTS ... Multimedia)`。
**模块缺失会让整个 APK 构建在 configure 阶段失败**，而不是只让朗读不可用。

```bash
ls /opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6Multimedia
```

不存在就用 Qt Maintenance Tool 补装 Multimedia。对应工单 `issues/01`。

### 3.2 `QAudioSink` 是否接受 8 kHz —— 这条决定一段代码去留

模型真实采样率是 **8000 Hz**，规格要求按它播放。但 `QAudioSink` **不做采样率转换**：
格式不被设备支持就是没声音。而 Android 原生输出通常是 48 kHz。

实施者在**没有实机的情况下加了兜底**：若 `isFormatSupported()` 不认 8 kHz，就退回设备
采样率，并由 `resampleLinearLe()`（`src/mainwindow.cpp` 文件内匿名命名空间的自由函数）
做线性插值重采样。

**请把运行日志里这一行原文抄给发起人：**

```
音频输出已打开 <rate> Hz 单声道 int16 重采样: <true|false>
```

- `重采样: false` → 设备直接支持 8 kHz，**那段兜底代码可以删掉**（`resampleLinearLe`
  与两个 int16 辅助函数，共 44 行，其中 37 行代码；另加 `setupAudioOutput()` 里约
  12 行的分支与 `m_needResample` 的处理）
- `重采样: true` → 兜底是必需的，请**重点听音质**：线性插值对语音够用，但如果你听出
  明显失真/金属声，说明需要换成更高阶的重采样

对应工单 `issues/04`。

---

## 4. 验收清单（对应 `issues/01`–`07`）

工单文件在 `.scratch/offline-tts/issues/`，每条都写了判据与做法。
下表是索引，**判据原文引自 `spec.md`，不要降低标准。**

| # | 判据（引自 spec） | 怎么看 |
|---|---|---|
| 01 | Qt 6.11.1 Android 套件含 Multimedia | 见 3.1 |
| 02 | 首次启动后目标文件出现；第二次启动不重复复制；**手工杀掉进程再启动，不会留下被误判为有效的半截文件** | 第 3 条最重要：复制中途 `adb shell am force-stop <包名>`，重启后检查 `/data/data/<包名>/files/tts-vits-icefall-zh-aishell3/` 有无残留 `.tmp`、应用是否**重新完整提取**而不是直接认为就绪 |
| 03 | `AppDataLocation` 下**不再出现** whisper 模型文件，而「转写」仍然工作 | 先在旧版本上装一次让 `model.bin` 落盘，再装新版本；确认它不再被写入。注意新代码**不会去删旧文件**，所以别把历史遗留误判为"还在落盘" |
| 04 | 能听到声音；**长回复播放途中「停止」立即生效** | 见 3.2。停止走 `QAudioSink::reset()`（丢弃缓冲）而非 `stop()`（会排空缓冲），所以应当立即静音 |
| 05 | APK 构建通过；对着麦克风说一句英文，得到中文回复**并被朗读出来** | 端到端。同时确认三条**静默路径**都不出声：回复为空 / `[无识别结果]` / `应答`失败 |
| 06 | `ctest --test-dir build-tests` 的 25 条用例 | 见下 |
| 07 | 记录真实 APK 体积增量 | 见下 |

### 跑单元测试（这是本仓库第一个能离线跑的测试）

```bash
cmake -B build-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

该目标**只在非 Android 构建生成**（根 `CMakeLists.txt` 末尾），用 host Qt，不需要设备或 NDK；
它只编译被测的那一个 `.cpp`，不链接整个 app（app 依赖 POSIX 头）。

> ⚠️ 实施者已用**独立 Python 移植**跑过 25 条同表用例且全过，但**那验证的是算法，不是
> C++ 代码**。C++ 侧从未编译过，出现编译错误是完全可能的。

### 记录 APK 体积增量

```bash
git stash        # 或 git checkout snapshot/pre-tts-implementation
# 构建一次，记下体积
git checkout feature/ai-text-to-speech
# 再构建一次，记下体积
```

可计算的等价数字（未压缩）是 **57.38 MB**：
`libsherpa-onnx-c-api.so` 4.46 + `libonnxruntime.so` 21.68 + 模型 31.24。
真实增量应明显更小（APK 内压缩率不同）。

---

## 5. 已知问题、坑，以及**请不要做的事**

### 5.1 签名密钥已公开泄露（重要）

`android_release.keystore` 与 `.vscode/settings.json` **被跟踪且仓库是 public**，
后者含明文 `STORE_PASS`/`KEY_PASS`/`ALIAS`（都是 `smartglasses`）。
发起人已确认知晓此情况**并选择暂不处理**，但你必须知道：

- **验证请用 Debug 构建**，它不需要 keystore
- 若确需 Release 构建：用仓库里这把 key 能保持与既有安装包的升级兼容性
  （换 key 会导致 `INSTALL_FAILED_UPDATE_INCOMPATIBLE`），但**这把私钥已泄露**，
  用它签出来的包不具安全性
- **不要**把它复制到别的地方，也不要基于它做任何"正式发布"

### 5.2 不要「顺手修」这些

| 不要动 | 为什么 |
|---|---|
| `whisper_manager.cpp` 的 `params.language = "en"` | **刻意选择**。使用者说英文，whisper 英文识别率更高；回复之所以是中文由 `msgsender.cpp` 的中文 system prompt 决定（API 路径 :88，Ollama 路径 :151）。改成 `zh`/`auto` 会让转写质量下降且原因不出现在报错里。附近有长注释说明 |
| 把 `rule.far` 加回模型目录 | 它 172.3 MB，而官方文档对该模型**只传** `phone.fst,date.fst,number.fst`，从不引用它。C API 里 `rule_fsts` 与 `rule_fars` 是两个独立字段，代码只用前者。加回去会让 APK 白胖 172 MB |
| 为播放器引入抽象接口/多实现 | 规格明确列为「刻意不做」：**一个 adapter 是假设的 seam，两个才是真的**。现在只有一个真实变体（手机扬声器）。蓝牙侧硬件定下来后再做 |
| 把 `.so` 改成提交进 git | 是发起人明确的决定（只提交模型，`.so` 走脚本），见 `third_party/sherpa-onnx/README.md` |

### 5.3 版本相关的坑

- **`QAudio` 命名空间在 Qt 6.6 之后改名为 `QtAudio`**。代码里刻意**不比较** `state()` 枚举，
  而是无条件调用 `m_audioSink->resume()`（文档写明非挂起状态下它是空操作）。
  这样写同时兼容新旧 Qt，请不要"优化"成枚举比较。
- `qMin` 是**单模板参数**，两个实参类型必须一致（`qsizetype` vs `int` 会推导失败）。
- `.gitattributes` 是**必需的**：`core.autocrlf=true` 会把 sherpa 逐行解析的
  `lexicon.txt`/`tokens.txt` 检出成 CRLF，静默劣化合成质量。不要删。

### 5.4 与本次改动无关的既有垃圾

- `backfill.log`（12 KB，未跟踪）——不是本次产物，忽略即可
- 仓库里另有既有的 P0 问题（上述密钥）与 P1 问题（`third_party/android_openssl`
  只有头文件、没有预编译 `.so`，被 `.gitignore` 的 `*.so` 挡在门外）。
  **这些不是本次改动引入的**，别顺手去修，那会扩大你的验证面。

---

## 6. 改动概览（便于你判断风险）

| 步骤 | 产物 | 风险点 |
|---|---|---|
| 1 | `third_party/sherpa-onnx/`、`android/assets/models/tts-vits-icefall-zh-aishell3/` | CMake 的 IMPORTED 目标 + `QT_ANDROID_EXTRA_LIBS` 打包两个 `.so`；**从未链接过，是首要风险** |
| 2 | `includes/assetextractor.h`、`src/assetextractor.cpp` | 后台线程 + 分块复制 + `.tmp`+rename + 清单最后写 |
| 3 | `src/whisper_manager.cpp` | 改用 `whisper_init_from_buffer_with_params`；已核实该函数不保留调用方 buffer（`whisper.cpp:3668-3703`，`buf_context` 在栈上），所以 init 后即可释放 |
| 4 | `includes/speechsynthesizer.h`、`src/speechsynthesizer.cpp` | sherpa C API 调用、有界 PCM 队列、按代次取消 |
| 5 | `includes/sentencesplitter.h`、`src/sentencesplitter.cpp`、`tests/` | 纯自由函数，算法已离线验证 |
| 6 | `src/mainwindow.cpp`（播放部分）、`qml/Main.qml` | `QAudioSink` push 模式；**排空定时器 + 低水位背压 + 设备无响应检测**，这套逻辑是审查后修的，最值得你带着怀疑去看 |
| 7 | `src/mainwindow.cpp`（`onAIResponse`） | 三条静默路径 |
| 9 | `src/whisper_manager.cpp`（注释） | 无风险 |

**实现细节与三处决策修订**（为什么没用"静态链接"产物、为什么剔除 `rule.far`、
为什么加了重采样）都记在 `.scratch/offline-tts/spec.md` 的「实施期修订」与第六节，
含 sha256 与资产逐个核实的结论。

---

## 7. 回执模板

请按这个格式回给发起人（哪怕中途失败也请回执，失败信息比成功更有价值）：

```
环境：Qt 6.11.1 / NDK 27.2.12479018 / 设备型号 ____ / Android ____

1. Qt Multimedia 是否存在：是 / 否
2. 音频日志那行原文：音频输出已打开 ___ Hz ... 重采样: ___
3. 编译是否通过：是 / 否
   若否，完整错误（尤其是链接错误、c-api.h 相关、QAudioSink 相关）
4. 05 端到端：能否听到中文朗读：是 / 否
   「停止」是否立即生效：是 / 否
5. 06 ctest 结果：__ 条通过 / __ 条失败（失败请贴出断言）
6. 07 APK 体积：改动前 ___ MB / 改动后 ___ MB
7. 其他异常、崩溃、日志（有就贴，没有就说没有）

对代码的修改：无 / 有（请贴 diff 并说明理由）
```

---

## 8. 如果你卡住了

| 症状 | 先看这里 |
|---|---|
| configure 报 sherpa-onnx 缺失 | 忘了跑 `download-libs.sh`；报错信息里有路径提示 |
| configure 报 Qt6Multimedia 找不到 | 见 3.1，用 Maintenance Tool 补装 |
| `git clone` 超时 | 见第 1 节的代理说明 |
| 链接期找不到 `SherpaOnnxCreateOfflineTts` | 确认 `libsherpa-onnx-c-api.so` 在 `jniLibs/arm64-v8a/`，且 `CMAKE_ANDROID_ARCH_ABI` 是 `arm64-v8a` |
| 有声音但短回复只响开头就没了 | 这正是审查抓到的 bug。若仍复现，检查 `m_audioPump` 是否在跑、`bytesFree()` 是否一直为 0 |
| 界面永远停在「正在朗读」 | 检查是否触发了设备无响应检测（3 秒无写入即放弃） |
| 合成完全没声音 | 确认 `model.onnx`/`lexicon.txt`/`tokens.txt` 三个文件都被提取到了 `AppDataLocation` |

代码里所有非显然的设计都写了「为什么」，搜索 `// 注意`、`// 刻意`、`// 为什么` 能找到它们。
