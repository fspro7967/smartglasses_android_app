# 规格:离线中文语音合成与手机端朗读

**状态:** 决策已冻结,待实施
**日期:** 2026-09-10
**来源:** 由一次 grilling 会话产出,决策集已由仓库所有者逐条核对确认

---

## 这份文档要做什么

给这个 app 加上**合成**:把 `应答` 产出的中文文本变成语音,并在**手机扬声器**上播放。

**不在范围**(刻意不做,见下方「为什么不做」):

- 蓝牙回写、下行协议、眼镜侧播放
- 播放器抽象层、音频编码/压缩

---

## 一、冻结的决策

实施时不重新讨论这些。需要变更就先改这份文档。

| 项 | 决定 |
|---|---|
| 首版出声位置 | **手机扬声器**。不碰蓝牙 |
| 引擎 | **sherpa-onnx** + **`vits-icefall-zh-aishell3`** |
| 模型 | `model.onnx` 29.07 MB；**vendored 后 31.24 MB**（见下方「实施期修订」） |
| 链接方式 | ~~静态链接 ONNX Runtime 的构建产物(34.3 MB tar.bz2,单个 `.so`)~~ → **非静态 `android.tar.bz2` 的两个 `.so`**（见下方「实施期修订」） |
| API | 纯 C API(`libsherpa-onnx-c-api.so`),**不引入 JNI** |
| 合成粒度 | **逐句**。按 `。！？；…` 与换行切分,设最小长度阈值,**不在数字中间切** |
| 播放 | `应答` 到达**自动朗读**;提供**停止/重播** |
| 音频输出 | **新增 `Qt6::Multimedia`**,`QAudioSink` push 模式 |
| 流水线语言 | 输入**英文**(`whisper_manager.cpp:88` 的 `"en"` 保持不变),回复**中文**,朗读**中文** |
| 静默策略 | 空回复 / `[无识别结果]` / `应答`失败 —— **一律不出声** |
| 模型打包 | 后台线程、分块复制、`.tmp` + rename、按字节长度校验、文件名带模型标识 |
| whisper 模型 | 改用 `whisper_init_from_buffer_with_params`,**不再解压到磁盘** |
| 授权 | 个人使用 | 

### 实施期修订（2026-09-12）

以下三条是在实施步骤 0 核实实物后**必须**改的，原决策在 v1.13.7 的已发布产物里不成立：

1. **链接方式：静态 → 非静态。** 标称「静态链接 ONNX Runtime」的产物
   （`sherpa-onnx-v1.13.7-android-static-link-onnxruntime.tar.bz2`，34.3 MB，
   字节数与原决策完全一致）里**只有 `libsherpa-onnx-jni.so`**，没有 C API 库。
   已对 289 个 release 资产逐个核实：**唯一含 `libsherpa-onnx-c-api.so` 的是非静态的
   `sherpa-onnx-v1.13.7-android.tar.bz2`（45.2 MB）**。
   即「静态链接」与「纯 C API、不引入 JNI」在已发布产物里互斥。
   本仓库是纯 C++、零 Java、无 gradle 文件，引入 JNI 是架构级改动；而放弃静态链接
   只多 2.38 MB（24.93 MB vs 22.55 MB）。**保住不引入 JNI，放弃单个 `.so`。**
   如将来确实需要单个 `.so`，须在构建机上从源码构建
   （`-DSHERPA_ONNX_ENABLE_C_API=ON -DBUILD_SHARED_LIBS=ON` + 静态 ONNX Runtime）。

2. **模型：30 MB → 31.24 MB（剔除 `rule.far`）。** 官方归档解压后其实是
   **203.59 MB**，其中 `rule.far` 一个文件占 **172.3 MB**。但官方文档对该模型只传
   `--tts-rule-fsts=phone.fst,date.fst,number.fst`，**从不引用 `rule.far`**；
   C API 里 `rule_fsts` 与 `rule_fars` 也是两个独立字段，本项目只用前者。
   剔除 `rule.far` 后正好是原决策写的「30 MB」，说明原决策的意图就是不含它的形态。

3. **新增重采样兜底（原决策未涉及）。** `QAudioSink` **不做**采样率转换，
   格式不被设备支持就是没声音；Android 原生输出通常是 48 kHz，而
   `QAudioDevice::isFormatSupported()` 未必认 8 kHz——那会让步骤 6 的「能听到声音」
   判据直接不成立。因此 `setupAudioOutput()` 在 8 kHz 不被支持时退回设备采样率，
   由 `mainwindow.cpp` 里的 `resampleLinearLe()` 补一次线性插值转换。
   这是实施期新增的代码，不是原决策的一部分。

另外，原决策里的 `.so` 打包方式明确为：**预编译 `.so` 不入版本库**
（`.gitignore` 全局排除 `*.so`），由 `third_party/sherpa-onnx/download-libs.sh`
在构建机上拉取并校验 sha256；**模型入库**（与既有 whisper `model.bin` 的做法一致）。


**为什么不选其他引擎** —— 完整的十五个候选逐条淘汰与出处见
`.scratch/offline-tts/offline-chinese-tts-comparison.md`。实施时不必重读,除非要换引擎。

---

## 二、实施步骤

每一步以**完成判据**收尾。判据分两类:

- **[静态]** —— 读代码/看文件就能判定,在这台 Windows 机器上可完成
- **[构建机]** —— 需要 Linux + Android SDK + Qt 的构建机才能判定

> **重要:** 本仓库在这台 Windows 机器上**编译不了**——工具链是 Linux/Android 形态
> (`CMakeLists.txt:14` 探测 `/opt/Qt/6.11.1/gcc_64`,`whisper_manager.h:7` 引入 POSIX `<sys/resource.h>`)。
> 因此所有 **[构建机]** 判据必须由持有构建机的人来跑。

### 步骤 0 — 清掉两个阻塞项

这两件事没确认之前,后续步骤有可能整段白做。

1. 确认 Qt 6.11.1 的 Android 套件里**有 Multimedia 模块**:
   检查 `/opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6Multimedia` 是否存在。没有就用 Qt Maintenance Tool 补装。
2. 下载 sherpa-onnx 的 Android **静态链接 ONNX Runtime** 产物,并确认它**确实包含非 JNI 的 C API 库**
   (`libsherpa-onnx-c-api.so`)。发布页的构建脚本 README 声明它有,但需开箱核实。
3. 下载 `vits-icefall-zh-aishell3` 模型。

**完成判据 [静态]:** 三个产物都在本地;`libsherpa-onnx-c-api.so` 已确认存在。
任一项缺失即在此步停下并报告缺哪一项。

### 步骤 1 — vendor sherpa-onnx 与模型

照抄现有的 vendored 库模式(`CMakeLists.txt:37-38` 的 `add_subdirectory`、`:98` 的链接、`:101-103` 的 include 路径)。

- 预编译 `.so` 与头文件放到 `third_party/sherpa-onnx/`
- 模型放进 `android/assets/models/`(`android/` 已由 `CMakeLists.txt:67-68` 的
  `QT_ANDROID_PACKAGE_SOURCE_DIR` 打包)

**完成判据 [静态]:** `CMakeLists.txt` 中出现 sherpa-onnx 的 include 与链接;
`android/assets/models/` 下存在 TTS 模型;**记录 APK 体积增量(改动前/后各一个数字)**。

### 步骤 2 — TTS 模型的解压

新增一个解压辅助函数(不要改 `mainwindow.cpp:314-347` 的 `extractModelToFile`,它服务于 whisper,
而这个新函数在步骤 3 之后会成为唯一还在解压的路径)。

四件事,顺序不能变:

1. 在**后台线程**执行,不占用构造函数
2. **分块复制**(约 1 MB 一块)—— 现有代码用 `readAll()` 一次性读入 59.7 MB,有被 Android 低内存杀手干掉的风险
3. **先写 `<name>.tmp`,写完后 `rename`** —— 保证进程被杀时不会留下「看起来有效」的半截文件
4. **按字节长度校验**是否存在,取代现有的「文件存在即有效」(`mainwindow.cpp:322-325`)

**完成判据 [构建机]:** 首次启动后目标文件出现;第二次启动不重复复制;
**手工杀掉进程再启动,不会留下被误判为有效的半截文件**。

### 步骤 3 — whisper 改为从内存加载

`whisper.h:207` 提供了:

```c
whisper_init_from_buffer_with_params(void * buffer, size_t buffer_size,
                                     struct whisper_context_params params);
```

改为从 `assets:/models/model.bin` 读入内存后直接初始化,**不再写盘**。

这一步省掉首次运行的 59.7 MB 写入、省掉一份重复存储,并从根上消除半截文件风险。
它与步骤 2 相互独立,可以单独完成。

**完成判据 [构建机]:** `AppDataLocation` 下**不再出现** whisper 模型文件,而 `转写` 仍然工作。

### 步骤 4 — 合成模块

新增 `includes/speechsynthesizer.h` 与 `src/speechsynthesizer.cpp`,与 `WhisperManager` 对称。

**接口纪律** —— 这一条是硬要求,因为 `WhisperManager` 在这两点上做错了:

| 要求 | 反例(照抄会重犯) |
|---|---|
| 静默是**一种正常结果**,不是错误 | `whisper_manager.cpp:134-135` 把「无识别结果」同时报成 error 和一个文字哨兵值 |
| 进度**不走** error 通道 | `whisper_manager.cpp:49` 用 `errorOccurred` 上报日常喂入进度 |
| `init` 的结果**必须被消费** | `mainwindow.cpp:63-67` 拿到 init 失败后仍继续接活,错误随后按块重复涌现 |

接口形状:

- `init(modelDir)`,并暴露可查询的就绪状态,让调用方能**拒绝**在未就绪时调用合成
- `synthesizeSentence(text)`
- 信号:`audioChunk(QByteArray pcm, int sampleRate)`、`sentenceFinished()`、`failed(QString reason)`

用 sherpa C API 的**增量回调**(它返回 `SherpaOnnxGeneratedAudio{const float *samples; int32_t n; int32_t sample_rate;}`,
单声道 float,PCM 范围 `[-1, 1]`),边合成边发 `audioChunk`,不必等整句完成。

合成在 worker 线程(与现有 `QtConcurrent` 用法一致),**PCM 队列设上限**。

**完成判据 [静态]:** 读完头文件,每个成员都能指出它的调用方;
模块在**正常路径上不发出任何 `failed`**。

### 步骤 5 — 逐句切分

一个**纯函数**:`QList<QString> splitSentences(const QString&)`。

按 `。！？；…` 和换行切分;设最小长度阈值;**不在数字中间切**(否则 `3.14` 会被切成两句)。

**完成判据 [静态]:** 它是自由函数,不依赖任何 QObject / 信号槽 —— 这是本仓库里
**第一个不需要真实设备就能测试的逻辑**,值得为此写测试。

### 步骤 6 — 播放

- `CMakeLists.txt` 新增 `Qt6::Multimedia`
- 用 `QAudioSink` push 模式播放;`QAudioFormat` 用模型真实采样率(8 kHz)与 int16;float → int16 转换
- 在「AI 回复」页接上:自动播放 + 停止 + 重播

**完成判据 [构建机]:** 能听到声音;**长回复播放途中「停止」立即生效**。

### 步骤 7 — 静默策略与接线

在 `MainWindow::onAIResponse`(`mainwindow.cpp:304-307`)接入,并守住三条静默路径:

- 回复文本为空 → 不合成
- `[无识别结果]` → 不合成(它已在 `mainwindow.cpp:297` 被挡在 `应答` 之外)
- `应答`失败 → 不合成,只在界面与日志显示

**完成判据 [静态]:** 三条路径逐条走查,均有明确分支;
**读代码可确认没有任何一条会走到 `synthesizeSentence`。**

### 步骤 8 — 在构建机上端到端验证

**完成判据 [构建机]:** APK 构建通过;对着麦克风说一句英文,得到中文回复**并被朗读出来**。

### 步骤 9 — 记录语言不对称

在 `whisper_manager.cpp:88` 的 `params.language = "en";` 旁边加一句注释,说明这是**刻意选择**:
使用者说英文,且 whisper 的英文识别率更高(回复语言是中文,由 `msgsender.cpp:88` 的中文 system prompt 决定)。

不加注释的话,后人会把它当成 bug「顺手修掉」,然后转写质量下降而原因不明。

**完成判据 [静态]:** 注释存在,且解释了「为什么是 en 而不是 zh/auto」。

---

## 三、刻意不做的事

这三项都是**为尚不存在的变体预留抽象**。硬件定下来、第二个变体真正出现时再做。

| 不做 | 何时做 |
|---|---|
| 播放器抽象接口 + 多个实现 | 眼镜侧硬件确定后。届时才出现第二个真实变体 |
| 音频编码/压缩(ADPCM 等) | 蓝牙回写提上日程时 |
| 蓝牙下行协议与可写特征 | 眼镜侧固件存在后 |

**理由(记录一次,以后不再重复论证):** 一个 adapter 是假设的 seam,两个才是真的。
现在只有一个真实变体(手机扬声器),所以不建 seam。

**已经确认的相关事实,供将来做蓝牙时直接取用:**

- Qt **能读 MTU 但不能请求**:`QLowEnergyController::mtu()` 与 `mtuChanged()` 存在,
  但公开接口里**没有** `requestMtu()`;单包最大数据量是 `mtu - 3`,连接前默认返回 `23`
- 外设侧(Arduino)可以主动发起 MTU 交换,所以这个约束应由固件侧解决
- 8 kHz 输出让未来的码率账好算得多:16 kHz 裸 PCM 是 32 KB/s,
  **8 kHz 裸 PCM 是 16 KB/s,8 kHz + IMA ADPCM 只需约 4 KB/s**

---

## 四、已知风险

| 风险 | 状态 |
|---|---|
| **没有任何 Android arm64 的实测 RTF** | 所有公开数字都是 x86 或树莓派 4。性能要等真机 |
| Android tarball 是否真含非 JNI 的 C API 库 | ✅ **已核实：静态版不含**，只有 JNI 库；改用非静态版（见「实施期修订」1） |
| Qt Multimedia 是否已安装 | ⚠️ **仍未验证**。实施本机无 Qt，无法确认；构建机需检查 `/opt/Qt/6.11.1/android_arm64_v8a/lib/cmake/Qt6Multimedia` |
| 本仓库在此机器编译不了 | ✅ 已确认（无 Qt / 无 CMake / 无 C++ 编译器），所有 **[静态]** 判据已在此机完成，**[构建机]** 判据待跑 |
| `.ts` 翻译目录是空的 | `qsTr()` 与 `tr()` 目前全是空操作(见 `smartglasses_android_app_zh_CN.ts`)。新增字符串沿用现有做法即可,不要以为现有的 i18n 是生效的 |
| ~~仓库不是 git 仓库~~ | ✅ 已是 git 仓库。实施前已存快照 tag `snapshot/pre-tts-implementation`(→`5d21a03`),改动在分支 `feature/ai-text-to-speech` 上进行 |
| QAudioSink 是否接受 8 kHz | ⚠️ 未知。`QAudioSink` 不做重采样，已加线性插值兜底（见「实施期修订」3），但真机上仍需确认走的是哪条路径 |
| whisper 初始化仍在构造函数里 | ⚠️ 已知。步骤 3 只去掉了解压，`whisper_init_from_buffer_with_params` 同步解析 57 MB 模型仍在 `MainWindow` 构造函数中，会拖慢首屏。规格未要求改，故未改 |

---

## 五、相关文件

| 用途 | 路径 |
|---|---|
| 引擎淘汰与出处的完整依据 | `.scratch/offline-tts/offline-chinese-tts-comparison.md` |
| 附带调研材料 | `.scratch/tts-research/`、`.scratch/research/`、`.scratch/piper-embedding-research.md` |
| 本次同时发现的架构问题与缺陷清单 | 见 `/improve-codebase-architecture` 报告(临时目录,未入库) |

---

## 六、实测数据

由工单 01 / 06 / 07 在 Linux 构建机上量得并回填。**这三项判据已闭合**;
其余 [构建机] 判据（工单 02 / 03 / 04 / 05）仍需真机,状态见各工单文件。

### 6.1 APK 体积增量（工单 07）

| 项 | 字节 | MiB |
|---|---:|---:|
| 改动前（无 TTS） | 86,019,540 | 82.03 |
| 改动后（含 TTS） | 132,887,802 | 126.73 |
| **增量** | **+46,868,262** | **+44.70（+54.5%）** |

两次构建同一工具链、同一 ABI、同一构建类型（Release，arm64-v8a，NDK 27.2.12479018）。
实测增量约为按未压缩体积估算值（57.38 MB）的 0.78 倍。

### 6.2 单元测试（工单 06）

`tst_sentencesplitter` 在 host Qt（`/opt/Qt/6.11.1/gcc_64`）上 26/26 通过，0 失败。

### 6.3 依赖与构建前置（工单 01、步骤 0）

- Qt6Multimedia 六件套在 `android_arm64_v8a` 下齐全，且 `libQt6Multimedia_arm64-v8a.so`
  确实进入 APK 的 `lib/arm64-v8a/`——工单 01 的「模块缺失会让 configure 阶段失败」
  这一风险不复存在。
- 步骤 0 的第 2 条结论要修正：标称「静态链接 ONNX Runtime」的产物只有 JNI 库，
  因此改用**两个 .so** 的那份产物（这一判断已写进 `third_party/sherpa-onnx/README.md`
  与 `sherpa-onnx.cmake` 顶部注释）。
- `download-libs.sh` 有两处只有在 Linux 上才会暴露的问题，均已修复：
  1. 归档成员名带 `./` 前缀，GNU tar 不像 bsdtar 那样自动等价匹配，原地报
     「归档中找不到」，需写成 `"./jniLibs/<abi>/<file>"` 且 `--strip-components=3`；
  2. release 资源实际由 `objects.githubusercontent.com` 提供，国内直连**一个字节都
     下不来**；新增可选镜像前缀 `SHERPA_ONNX_MIRROR`（sha256 校验照旧执行）。

### 6.4 合并状态

实现分支 `feature/ai-text-to-speech` 已并入 `master`（合并提交 `886d05c`）。
`master` 上的 **BLE 回写**功能与本规格的 TTS 功能同时存在——冲突只限
`includes/mainwindow.h` 与 `src/mainwindow.cpp`（两者都动过同一批成员与 `onAIResponse`）。
取舍：`onAIResponse()` 先走静默策略，再 BLE 回写，最后逐句朗读；静默返回同时跳过
BLE 回写（空回复与 `[无识别结果]` 都没有下行价值）。

注意：这使本规格第三节「刻意不做的事」里的**蓝牙下行与可写特征**事实上已由另一条
工作线落地（早于 TTS 合并）。该节记载的「何时做」判断因此已过期，但留档不改——
它是当时的决策记录。
| 引擎/模型来源、校验和与选型理由 | `third_party/sherpa-onnx/README.md` |

---

## 六、实施状态（2026-09-12）

分支 `feature/ai-text-to-speech`，改动前已存快照 tag `snapshot/pre-tts-implementation`。

### 已完成的 [静态] 判据

| 步骤 | 产物 | 判据 |
|---|---|---|
| 0 | 两个归档 + `c-api.h` 下载完成 | ✅ 字节数与 sha256 逐一核对；**发现静态版无 C API 库**，已报告并由所有者选定非静态方案 |
| 1 | `third_party/sherpa-onnx/`、`android/assets/models/tts-vits-icefall-zh-aishell3/` | ✅ CMake 中出现 include 与链接；模型就位；APK 体积见下 |
| 2 | `includes/assetextractor.h`、`src/assetextractor.cpp` | ✅ 后台线程、1 MiB 分块、`.tmp`+rename、字节长度校验、清单最后写 |
| 3 | `whisper_manager.cpp` 改 `whisper_init_from_buffer_with_params` | ✅ 已核实该函数不保留调用方 buffer（内部 `buf_context` 在栈上），故 init 后即可释放 |
| 4 | `includes/speechsynthesizer.h`、`src/speechsynthesizer.cpp` | ✅ 每个成员都有调用方；正常路径不发 `failed` |
| 5 | `includes/sentencesplitter.h`、`src/sentencesplitter.cpp`、`tests/` | ✅ 纯自由函数；**新增本仓库第一个可离线运行的单元测试** |
| 6 | `Qt6::Multimedia` + `QAudioSink` | ✅ 代码就位（含采样率兜底）；**能否出声待真机** |
| 7 | `onAIResponse` 三条静默路径 | ✅ 逐条走查，均不会到达 `speakReply()` |
| 9 | `whisper_manager.cpp` 语言不对称注释 | ✅ 已注明为何是 `en` 而非 `zh`/`auto` |

### APK 体积增量

本机无法产出 APK，故给出**可计算的等价数字**（未压缩）：

| 项 | 体积 |
|---|---:|
| `libsherpa-onnx-c-api.so` | 4.46 MB |
| `libonnxruntime.so` | 21.68 MB |
| TTS 模型（assets，剔除 `rule.far`） | 31.24 MB |
| **合计增量** | **57.38 MB** |

真实 APK 增量需在构建机上量（`assets` 与 `lib` 在 APK 内的压缩率不同，且
移除 whisper 落盘不影响 APK 体积）。**这是步骤 1 判据中尚未闭合的一项。**

### 超出原步骤清单的改动（如实记录）

以下几项不在 spec 的步骤 0–9 里，是实施时判断必要性后加入的：

| 改动 | 为什么加 |
|---|---|
| 新增 `.gitattributes` | 开发机全局 `core.autocrlf=true` 会把 sherpa 逐行解析的 `lexicon.txt`/`tokens.txt` 检出成 CRLF，也会破坏 `*.sh` 的 shebang——不加会静默损坏功能 |
| 新增 `tests/` 与根 `include(CTest)` | 步骤 5 只说「值得为此写测试」，这里把它落成可运行的 cmake 目标；该目标只在非 Android 构建生成，APK 不受影响 |
| 删除 `MainWindow::extractModelToFile()` | 步骤 2 说「不要改」它，但步骤 3 之后它已无调用方。留着就是死代码，且它「文件存在即有效」的判定正是规格点名要修的缺陷 |
| MainWindow 侧排空定时器 + 低水位背压 + 4 MiB 安全阀 | 规格只要求「PCM 队列设上限」。但合成比播放快约 6 倍：若只在收到音频块时才写设备，尾部音频永远写不出去（短回复**只听到开头**），长回复则堆到上限并谎报「音频输出停滞」。背压把上限还原成真正的安全阀；排空定时器则保证合成停下来之后积压仍能被写完 |
| 设备无响应检测（`kStalledTicksBeforeGivingUp`） | 背压闸门引入的新失败模式：设备卡住 → 积压不涨 → 安全阀永不触发 → 合成与播放双双静止、界面永远停在「正在朗读」。必须有个出口 |
| `SpeechSynthesizer` 的按代次取消（`m_cancelUpTo`） | 规格只要求「提供停止/重播」。若用单一取消标志，`stop()` 之后立刻开始下一句会让上一句的 worker 复活 |
| `resampleLinearLe()` | 见「实施期修订」3：不加则设备不认 8 kHz 时直接无声 |

### 尚未完成的 [构建机] 判据

以下 7 项已按 `docs/agents/issue-tracker.md` 的约定建为工单，
逐条可跟踪：`.scratch/offline-tts/issues/01-…07`（`Status: ready-for-human`）。

1. 步骤 0 之二：确认 Qt 6.11.1 Android 套件含 Multimedia 模块 → `issues/01`
2. 步骤 2：首次启动后目标文件出现；第二次启动不重复复制；**手工杀进程不留半截文件** → `issues/02`
3. 步骤 3：`AppDataLocation` 下**不再出现** whisper 模型文件，且转写仍工作 → `issues/03`
4. 步骤 6：能听到声音；长回复播放途中「停止」立即生效 → `issues/04`
5. 步骤 8：端到端——说一句英文，得到中文回复**并被朗读出来** → `issues/05`
6. 跑 `ctest --test-dir build-tests`（`tests/tst_sentencesplitter.cpp` 的 25 条用例） → `issues/06`
7. 记录真实 APK 体积增量 → `issues/07`
