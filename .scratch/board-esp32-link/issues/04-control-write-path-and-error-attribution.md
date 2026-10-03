# 04 — 控制指令写入不受队列记账约束,失败后果误伤音频队列且状态会撒谎

Status: needs-triage
Type: bug

## 问题一:控制写入的失败会清掉音频队列并中止回传

`DeviceHandler::onServiceError()`(`src/devicehandler.cpp:457-473`)用
「出错的 service 是不是当前的写入目标 service」来判定这次错误归不归它管:

```cpp
if (m_writeService != qobject_cast<QLowEnergyService*>(sender()))
    return;
...
const bool wasStreaming = m_streamQueuedBytes > 0 || m_streamOverflowed;
clearWriteQueue();
emit writeError(...);
if (wasStreaming)
    emit streamAborted(...);
```

开发板的三条特征在**同一个服务**里(`BoardProtocol::kServiceUuid`),所以
控制特征写入出错时 `sender()` 正好等于 `m_writeService`,判定通过。
后果:一次 `play`/`stop` 指令的失败,会把正在排队的音频分片全部丢掉,
并发 `streamAborted()` → `MainWindow` 关闭回传(`src/mainwindow.cpp:157-162`),
而状态栏给出的原因会是「蓝牙吞吐不足」或「写入失败」,指向错误的方向。

`writeControlValue()`(`src/devicehandler.cpp:228-260`)与 `writeData()`/`enqueueStreamData()`
是两条互不记账的路径:它不碰 `m_writeInFlight`、不碰 `m_writeQueue`,
`onCharacteristicWritten` 又会因 UUID 不同而忽略它的完成回调。

## 问题二:`play` 的成败无人知道,`m_boardPlaying` 会一直撒谎

`writeControlValue()` 只要 `writeCharacteristic()` 被调用就返回 true,
它无法知道这次写入最终成功与否。`boardPlayIfNeeded()`
(`src/mainwindow.cpp:503-521`)把 true 当成「板子已进入播放态」并置
`m_boardPlaying = true`。若这次写入随后失败:

- 固件没收到 `play`,于是**丢弃**播放特征上的所有音频(`main.cpp:180-183`)
- 应用侧仍认为在播放,继续喂音频,直到 128 KiB 积压触发 `streamAborted`
- 用户听到的是「什么都没有」,状态栏说的是「已开启语音蓝牙回传」

三处都指向同一件事:**控制指令这条路没有确认机制**。
固件其实有现成的回执 —— 控制特征收到 `play` 会 `notify("OK:playing")`
(`main.cpp:129-130`),收到 `status` 会回状态串(`main.cpp:153-162`)——
但应用侧只订阅了音频特征的通知,没有订阅控制特征,也从不解析回执。

## 可选方向(需要所有者定夺,故先 needs-triage)

1. **最小**:控制特征走独立记账 —— `writeControlValue()` 自己维护在途标志,
   失败只影响控制路径;`onServiceError()` 的判定加上特征 UUID,而不是只看 service。
2. **推荐**:订阅控制特征通知,把 `OK:playing` / `ERROR:*` 当作 `play` 的确认;
   在确认之前不置 `m_boardPlaying`,不喂音频。同时用 `status` 回执核对
   `mic:present` 与 `buffer:`。
3. 无论选哪个,`boardPlayIfNeeded()` 需要在「已发出但未确认」这个中间态下
   有一条超时 → 关闭回传并说明原因的出口(与 `issues/03` 的超时兜底同类)。

## 完成判据

- [ ] 一次控制写入失败不会清空/中止音频流,状态栏能指出是控制指令失败
- [ ] `play` 未确认(或超时)时不会置 `m_boardPlaying`,也不会继续喂音频
- [ ] `m_boardPlaying` 与实际固件状态在 `stop` 之后一致

## 相关

- 固件侧指令语义:`smartglasses_board/src/main.cpp:114-175`
- 共享服务的错误归属:`src/devicehandler.cpp:457-473`
- 自动配置:`src/mainwindow.cpp:469-535`
