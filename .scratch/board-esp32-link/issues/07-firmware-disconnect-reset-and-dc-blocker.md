# 07 — 固件:断开时不复位播放态;上行去直流滤波无饱和

Status: resolved
Type: bug

## 问题

`firmware/esp32c3/src/main.cpp` 里两处,2026-11-11 通读代码时发现:

1. **`ServerCallbacks::onDisconnect()` 不复位播放状态。** 手机在播放途中断开(走远、被系统杀掉)
   时 `stop` 指令不会送达,`isPlaying` 一直为 true、播放缓冲里留着残余样本。下次连接时固件
   会拒绝 `rate:N`(只允许在 stopped 时切换),残留音频还会在新会话开头被放出来。
2. **`applyDcBlocker()` 的输出直接截断成 int16。** 相邻样本差最大 ±65535,叠上滤波器记忆后
   会超出 int16;截断即回绕,正向大信号瞬间变成满幅负值,听起来是一声爆音,也会带偏识别。
   另外 `filteredState * R` 在状态接近 ±65535 时已经贴着 int32 上限。

## Answer

**2026-11-11 — 已修,固件两个环境在本机编译通过;未烧录、未在板子上验证。**

- `onDisconnect()`:`isPlaying = false; resetPlaybackBuffer();`。该回调与控制/播放回调同在
  BLE 任务里执行,可以直接动这些状态。
- `applyDcBlocker()`:输出用 `constrain()` 饱和到 int16,滤波器状态保留未饱和的值
  (只对输出限幅,否则会改变滤波器后续的响应);乘法改用 64 位。
- 文件头注释补上这两条,并写明**控制指令的回执文本是协议的一部分**——手机现在靠
  `OK:playing` 确认播放态(`issues/04`),改回执必须同步改 `BoardProtocol`。

验证:`pio run`(`esp32-c3-devkitc-02` 与 `esp32-c3-no-mic-detect`)均 SUCCESS。
烧录后的确认并入 `issues/05`(第 10、11 条)。

手机侧对第 1 点另有一层保险:每轮播放开头先发一条 `stop`(见 `issues/04`),所以即使板子上
还是旧固件,也不会卡在「拒绝 rate」上。
