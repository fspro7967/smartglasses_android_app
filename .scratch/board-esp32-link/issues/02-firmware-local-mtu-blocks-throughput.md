# 02 — 固件未切换 MTU + 播放特征只支持有响应写,下行带宽不成立

Status: ready-for-human
Type: task

这条是**阻塞项**:在修好之前,开发板的语音回传必定在几秒内自行中止。
修的位置在**固件仓库** `../../smartglasses_board`,不在本仓库。

## 结论(先用数字说)

| 项 | 值 | 依据 |
|---|---|---|
| 回传音频码率 | 16 kHz × 2 B = **32,000 B/s** | 固件 `SAMPLE_RATE 16000`(`main.cpp:17`) |
| 协商后的单包载荷 | **20 B**（MTU 23 − 3） | 见下 |
| 需要的写次数 | **1,600 次/秒** | 32000 ÷ 20 |
| 有响应写的上限 | 每个连接间隔 1 次 ≈ **≤133 次/秒** | WriteWithResponse 必须等 ACK |
| 实际上限字节率 | ≈ **2,660 B/s** | 133 × 20 |
| 需要的比例 | **约 8%** | 2660 ÷ 32000 |
| 应用侧积压上限 | 32,000 × 4 s = **128 KiB** | `src/devicehandler.cpp:306-313` |

按 8% 的达成率算,积压约 **4 秒**就撞上限;`enqueueStreamData()` 随即置
`m_streamOverflowed` 并发 `streamAborted()`,`MainWindow` 收到后自动关闭回传
(`src/mainwindow.cpp:157-162`)。界面上表现为「听几秒就停」。

## 为什么只有 20 字节

两件事叠加:

1. **Qt 侧其实会请求大 MTU。** `.scratch/offline-tts/spec.md` 第三节写
   「Qt 能读 MTU 但不能请求」,这在 Qt 6.11 上**不成立**:
   Qt 的 Android 后端在服务发现完成后会自己发起 `requestMtu(MAX_MTU)`,
   `MAX_MTU = 512`(`/opt/Qt/6.11.1/Src/qtconnectivity/src/android/bluetooth/src/
   org/qtproject/qt/android/bluetooth/QtBluetoothLE.java:53,344-357,1218-1228`)。
2. **固件把本地 MTU 留在 23。** arduino-esp32 的本地 MTU 默认 23,只有显式
   `BLEDevice::setMTU(mtu)` 才会抬高
   (`framework-arduinoespressif32/libraries/BLE/src/BLEDevice.cpp:548-558`,
   连接跟踪结构里也写死 `.mtu = 23`,同文件 `:618-628`)。
   `smartglasses_board/src/main.cpp` 里**没有**这个调用。
   于是 MTU 交换的结果是 min(512, 23) = 23,`writeChunkSize()` 返回 23 − 3 = 20。

即:手机端没做错什么,是固件没有参与协商。

## 修法(二选一或都做)

### A. 固件抬高本地 MTU(推荐,改动最小)

在 `setup()` 里、`BLEDevice::init()` 之后加:

```cpp
BLEDevice::setMTU(517);   // 或 512,需 ≥ 期望载荷 + 3
```

效果:MTU ≈ 512 → 载荷 509 B → 只需 **63 次写/秒**(约 16 ms 一次)。
这仍要靠连接间隔撑住,建议同时在连接后请求高优先级连接
(`esp_ble_gap_set_prefer_conn_params` / `updateConnParams`,间隔取 7.5–15 ms)。
**改完必须实测**:`DeviceHandler::writeData` 的 `qDebug` 会打印 `chunkSize`,
真机日志里应看到 500 左右而不是 20。

### B. 固件给播放特征加无响应写

```cpp
pPlaybackChar = pService->createCharacteristic(
    PLAYBACK_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR |   // 新增
    BLECharacteristic::PROPERTY_NOTIFY);
```

应用侧**不需要改**:`DeviceHandler::setWriteTarget()` 已经优先选
`WriteWithoutResponse`(`src/devicehandler.cpp:214-217`),那样发包节拍由
`streamTickIntervalMs()` 控制,不再被 ACK 往返锁死。

两条都做最稳:A 提供带宽余量,B 去掉 ACK 往返。只做 B 而 MTU 仍是 23 的话,
节拍被 `kStreamMinTickMs = 10` 钳住,上限 2,000 B/s,依然不够 —— 所以 **A 是必需项**。

## 完成判据

- [ ] 固件加入 MTU 设置并烧录
- [ ] 真机日志里 `DeviceHandler::writeData ... chunkSize =` 的值 ≥ 500
- [ ] 连续回传 30 秒以上不出现「已停止蓝牙音频回传: 蓝牙吞吐不足…」
- [ ] 板子上的扬声器听到完整、不欠载的语音(512 样本 = 32 ms 环形缓冲很小,
      欠载会表现为断续/爆音;必要时把 `PLAYBACK_BUFFER_SIZE` 提到 2048)

## 备注

- 本仓库不需要为此改动,除了把结论回填进 `issues/05`。
- 这条也回答了 `.scratch/ble-audio-out/issues/04` 的「实际吞吐」问题:
  对开发板而言,协商 MTU 由固件决定,预期是 500 左右;当前是 23。
