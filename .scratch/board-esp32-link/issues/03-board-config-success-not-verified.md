# 03 — 开发板自动配置宣告成功,但从不校验特征是否真的可用

Status: ready-for-agent
Type: bug

## 问题

`MainWindow::configureBoard()`(`src/mainwindow.cpp:469-489`)做三件事,然后无条件
发一条**成功**状态:

```cpp
m_board = true;
m_deviceHandler->enableCharacteristicNotification(   // 返回 void
    serviceUuid, kAudioCharUuid, true);
setWriteTarget(serviceUuid, kPlaybackCharUuid);      // 返回值被忽略
emit statusMessage("已识别开发板 ESP32_Audio:播放特征设为回传目标,麦克风通知已开启");
```

而 `DeviceHandler::enableCharacteristicNotification()`(`src/devicehandler.cpp:139-181`)
在所有失败路径上只做两件事:`qWarning()` + `return`,**不返回失败、不发信号、不改状态**:

- 服务找不到(`findService` 返回空)
- 服务对象存在但特征还没发现出来(`service->characteristic(...)` 无效)
- 特征没有 CCCD 描述符

所以「麦克风通知已开启」这句话可能是假的,而用户看不到任何异常:板子的上行是静的,
界面显示一切正常,唯一线索是 logcat 里一条 `qWarning`。

## 这条失败路径是真实存在的,不是假想

`DeviceHandler::onServiceStateChanged()`(`src/devicehandler.cpp:556-564`)把
`RemoteService` 状态也计入「详情已发现」:

```cpp
if (newState == RemoteServiceDiscovered || newState == RemoteService) {
    m_pendingServiceDetails--;
    ...
}
```

`RemoteService` 在 Qt ≥ 6.2 的语义是「**详情尚未发现**」(Qt 自己的文档与头文件注释:
"details are yet to be discovered";`DiscoveryRequired` 已改名为它)。该状态确实会作为
`stateChanged` 发出:当 `discoverServiceDetails()` 启动失败时,Qt 把状态从
`RemoteServiceDiscovering` 退回 `RemoteService` 并发出信号
(`/opt/Qt/6.11.1/Src/qtconnectivity/src/bluetooth/qlowenergycontroller_android.cpp:196-205`
        配 `qlowenergyserviceprivate.cpp:35-42`)。

也就是说:**开发板服务的详情发现一旦启动失败,它仍会被计入「已完成」**,
`serviceDetailsDiscoveryFinished()` 照常发出,`configureBoard()` 照常跑,
而 `service->characteristic(kAudioCharUuid)` 此刻是无效的 —— 走上面那条静默失败路径。

## 期望行为

成功状态必须是**校验过**的,而不是假设的:

1. `enableCharacteristicNotification()` 改为返回 `bool`(或在失败时发一个明确的错误信号),
   三个失败分支都要让调用方知道。
2. `configureBoard()` 逐项检查:
   - 服务存在 **且** 三个特征都 `isValid()`
   - 音频特征通知启用成功
   - 播放特征 `setWriteTarget()` 返回 true
3. 只有全部成功才置 `m_board = true` 并发「已识别开发板…」;
   否则发一条能看出**哪一步失败**的状态,并保持 `m_board = false`
   (退化成普通 BLE 设备的既有行为,用户可以手工选特征)。
4. 顺带:给服务详情发现加一条超时兜底。当前 `m_pendingServiceDetails` 只有在每个服务
   进入 `RemoteServiceDiscovered` 或退回 `RemoteService` 时才会递减
   (`src/devicehandler.cpp:556-564`);若某个服务卡在 `RemoteServiceDiscovering`,
   计数永远归不了零,`serviceDetailsDiscoveryFinished()` 永不发出,开发板就永远不会被识别
   —— 同样没有任何提示。

## 完成判据

- [ ] 断掉/伪造开发板音频特征的 CCCD,自动配置不会再说「麦克风通知已开启」,
      而是给出可定位的失败原因
- [ ] `m_board` 只在三个特征全部就绪后才为 true
- [ ] 服务详情发现加超时,超时后仍能走到一个明确的状态(成功或失败),不会静默卡住

## 相关

- 发现路径:`src/devicehandler.cpp:488-565`
- 使用方:`src/mainwindow.cpp:463-489`
- 阻塞它的带宽问题:`issues/02`
