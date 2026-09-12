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
| 模型 | 30 MB,8 kHz 单声道 |
| 链接方式 | **静态链接 ONNX Runtime** 的构建产物(34.3 MB tar.bz2,单个 `.so`) |
| API | 纯 C API(`libsherpa-onnx-c-api.so`),**不引入 JNI** |
| 合成粒度 | **逐句**。按 `。！？；…` 与换行切分,设最小长度阈值,**不在数字中间切** |
| 播放 | `应答` 到达**自动朗读**;提供**停止/重播** |
| 音频输出 | **新增 `Qt6::Multimedia`**,`QAudioSink` push 模式 |
| 流水线语言 | 输入**英文**(`whisper_manager.cpp:88` 的 `"en"` 保持不变),回复**中文**,朗读**中文** |
| 静默策略 | 空回复 / `[无识别结果]` / `应答`失败 —— **一律不出声** |
| 模型打包 | 后台线程、分块复制、`.tmp` + rename、按字节长度校验、文件名带模型标识 |
| whisper 模型 | 改用 `whisper_init_from_buffer_with_params`,**不再解压到磁盘** |
| 授权 | 个人使用 | 

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
| Android tarball 是否真含非 JNI 的 C API 库 | 仅官方构建脚本 README 声明,**步骤 0 会核实** |
| Qt Multimedia 是否已安装 | 本机无法验证,**步骤 0 会确认** |
| 本仓库在此机器编译不了 | 所有 **[构建机]** 判据需在 Linux/Android 环境执行 |
| `.ts` 翻译目录是空的 | `qsTr()` 与 `tr()` 目前全是空操作(见 `smartglasses_android_app_zh_CN.ts`)。新增字符串沿用现有做法即可,不要以为现有的 i18n 是生效的 |
| 仓库不是 git 仓库 | 改动前先 `git init`,否则无法 diff、无法回退 |

---

## 五、相关文件

| 用途 | 路径 |
|---|---|
| 引擎淘汰与出处的完整依据 | `.scratch/offline-tts/offline-chinese-tts-comparison.md` |
| 附带调研材料 | `.scratch/tts-research/`、`.scratch/research/`、`.scratch/piper-embedding-research.md` |
| 本次同时发现的架构问题与缺陷清单 | 见 `/improve-codebase-architecture` 报告(临时目录,未入库) |
