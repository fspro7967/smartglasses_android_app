# 06 — 设备意外断开后 DeviceHandler 不清理自身状态

Status: resolved
Type: bug

## 问题

主动断开走 `DeviceHandler::disconnectDevice()`,会清掉激活特征、写入目标、发送队列和服务对象。
**意外断开**(走远、对端掉电)只触发控制器的 `disconnected` 信号,而那个槽只做转发:

```cpp
connect(m_controller, &QLowEnergyController::disconnected, this, [this]() {
    emit statusChanged("Disconnected.");
    emit disconnected();
});
```

于是旧的 `QLowEnergyService` 对象、`m_writeService`、发送队列和定时器都留到下一次
`connectToDevice()` 才被清。另外三处相关的问题:

- `m_servicesDiscovered` 置 true 之后**从不复位**。下一次连接里,只要第一个服务的详情先于
  其他服务有结果,就可能被当成「全部发现完」,过早发 `serviceDetailsDiscoveryFinished()`
  (开发板自动配置随之过早运行)。
- 意外断开后的控制器不会再发 `disconnected`,下次 `disconnectDevice()` 却仍然
  「等它发信号再 deleteLater」——控制器对象一直留到应用退出。
- 用户断开 A 后立刻连接 B 时,A 迟到的 `disconnected` 会被原样转发,把 B 的界面状态
  (已连接、服务列表、写入目标)冲掉。

2026-11-11 通读代码时发现,此前没有工单。

## Answer

**2026-11-11 — 已修(代码在工作区,未经真机验证)。**

- 新增 `DeviceHandler::resetConnectionState()`,主动断开与意外断开共用:清激活特征、控制通道、
  写入目标与队列、在途记账、详情发现的待定集合与定时器,**复位 `m_servicesDiscovered`**。
  服务对象先断开信号再 `deleteLater()`(意外断开时本函数正处在控制器的信号发射途中,
  不能当场 delete)。
- `disconnected` 槽区分三种情况:仍是当前控制器 → 意外断开,复位并回收控制器;
  `m_controller` 为空 → 主动断开,只转发;已是另一个控制器 → 旧控制器迟到的事件,忽略。
  `connected` 与 `discoveryFinished` 也加了同样的「是不是当前控制器」判断。
- `disconnectDevice()` 对已经断开或从未连上的控制器直接 `deleteLater()`,
  链路已断时也不再去写「关闭通知」的描述符。

验证:Android Release 目标编译链接通过。真机判据并入 `issues/05`(第 9 条)。
