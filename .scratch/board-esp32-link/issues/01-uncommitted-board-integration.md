# 01 — 开发板适配改动未提交、无规格、无工单,需要决定走向

Status: needs-triage
Type: task

## 现状

工作区里有一版**未提交**的 ESP32-C3 开发板适配(`git diff`,6 个文件,+250/−20),
它做了这些事(细节见同目录 `spec.md`):

- 新增 `BoardProtocol` 协议常量(`includes/devicehandler.h:14-28`)
- `DeviceHandler` 新增 `servicePresent()` / `writeControlValue()` / `setStreamByteRate()`
- `MainWindow` 在服务详情发现完毕后自动识别开发板并配置三个特征
  (`src/mainwindow.cpp:463-489`)
- 回传音频按开发板要求做 8k→16k 线性重采样(`src/mainwindow.cpp:807-812`)
- 播放会话前后向控制特征发 `play` / `stop`(`src/mainwindow.cpp:503-535`)
- 开发板时不回写文本,只回传语音(`src/mainwindow.cpp:663-666`)
- QML 状态行区分「开发板」与「AI 回复输出」

静态验证:**能编译**。用 `builds/Qt-6.11.1-android_arm64_v8a-arm64/Release/compile_commands.json`
里的 Android 参数对三个改过的 .cpp 跑 `clang++ -fsyntax-only`,全部 exit 0。

## 问题

1. 改动没有任何工单或规格 —— 本目录的 `spec.md` 是**事后盘点**,不是所有者的决策。
   下一次 `git checkout` / `git stash` 就会把它弄丢,而且没人知道它是半成品还是成品。
2. 它推翻了 `.scratch/ble-audio-out/spec.md` 的一个前提:回传目标不再假定是眼镜,
   而是固件固定 16 kHz 的开发板。这条假设变化没有记在任何已冻结文档里。
3. 它同时夹带了与开发板无关的改动(`whisper_manager.cpp:121-129` 注释掉解码参数),
   应拆分,见 `.scratch/voice-pipeline/issues/03`。
4. 固件已经实现的 `volume:0-100` / `status` / `echo` 三条指令,手机侧一条都没用:
   - 界面没有音量控制(本机静音时,声音大小完全由固件默认值 180/255 决定)
   - 没有 `status` 查询,无法确认板子是否真的在播放、麦克风是否被识别为 present
   - `echo` 本来可以做一次「板子在不在、写通不通」的自检,也没用

## 需要所有者回答的

1. 这条路保留,还是回退?保留的话,是按 `spec.md` 的现状继续,还是先改设计?
2. 保留则需补一份真正的规格(冻结决策),并由本次盘点升格或重写。
3. 自动识别开发板是否符合预期?还是希望保留「用户手工选特征」这条既有通路即可?
   (当前实现是自动配置,用户在特征列表里再改会被 `setWriteTarget` 的守卫拦回)
4. 音量/状态/自检要不要做?做的话是界面控件还是自动轮询?

## 相关

- 阻塞项:`.scratch/board-esp32-link/issues/02`(固件 MTU,带宽不成立)
- 同批发现:`issues/03`、`issues/04`、`issues/05`
