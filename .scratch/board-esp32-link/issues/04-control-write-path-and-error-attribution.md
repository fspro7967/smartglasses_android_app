# 04 — 控制指令写入不受队列记账约束,失败后果误伤音频队列且状态会撒谎

Status: resolved
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

## Answer

**2026-11-11 — 按「可选方向」的 2(推荐)+ 3 实现,同时包含 1 的独立记账(代码在工作区,未经真机验证)。**

所有者的指示是「按工单修」,工单自己标了推荐方向,所以取了 2 + 3。

**问题一(错误归属)**

- `DeviceHandler` 新增显式的**控制通道**:`setControlChannel()` / `clearControlChannel()` /
  `writeControlValue(data)`,信号 `controlReply()` / `controlWriteFailed()`。
  控制指令优先用**有响应写**(可靠性优先于速度,和音频分片正相反)。
- 错误回调不带特征,所以归属靠**提交顺序**:`m_inflightWrites` 按顺序记下每一次已交给 Qt、
  尚未回调的写入属于哪条路(控制 / 普通下行 / 流式)。Qt Android 把同一连接上的读写串行
  执行,回调顺序即提交顺序(本机 Qt 源码 `QtBluetoothLE.java` 核对过:有响应与无响应两种写入
  都会收到完成回调)。出错时取该服务上最早的一条:是控制指令就只发 `controlWriteFailed()`,
  **不碰音频队列**。
- 控制特征的完成回调与通知都单独分流,不会混进下行队列或上行音频。

**问题二(`play` 要确认)**

- `m_boardPlaying` 换成三态 `BoardPlay { Idle, Requested, Playing }`。
- `requestBoardPlay()` 依次发 `stop` → `rate:N` → `play` 后进入 `Requested`。先发一条 `stop`
  是为了自愈:上一次的 `stop` 万一没送达,固件还停在播放态,会拒绝 `rate`。
- `Requested` 期间:回传音频扣在 `m_boardHeldPcm`,合成排空与本机 sink 一并暂停
  (`updateDrainGate()`),所以最多只多扣一两块,不会把整段回复堆进去。
- 收到固件的 `OK:playing`(`BoardProtocol::kReplyPlaying`)才转 `Playing` 并放行。
- 收到 `ERROR:*`、控制写入失败、或 2 秒(`kBoardPlayConfirmTimeoutMs`)没有确认:
  关闭回传、补发一条 `stop`、状态栏说明原因,**这条回复改由本机扬声器从头念完**(音频一块没丢)。
- `stop` 发不出去或写入失败时状态栏会提示「扬声器状态可能与界面不一致」,下一轮播放开头的
  那条 `stop` 会把两边重新对齐。

没有做的:用 `status` 回执核对 `mic:present` 与 `buffer:`(方向 2 的后半句),归入 `issues/08`。

验证:Android Release 目标编译链接通过;固件侧回执文本未改。三条完成判据都需要真机,
转入 `issues/05`。
