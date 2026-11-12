#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QBluetoothUuid>
#include <QMap>
#include <QSet>
#include <QElapsedTimer>

class QTimer;

// 开发板（ESP32-C3 蓝牙音频收发器，固件见 firmware/esp32c3/src/main.cpp）的协议常量。
// 开发板与手机各用一条特征：控制特征收发文本指令，音频特征上行麦克风 PCM，
// 播放特征下行扬声器 PCM。均为单声道 int16 小端；上行固定 16 kHz，下行采样率
// 由控制指令 rate:N 指定（见 kCmdRatePrefix），板子线性插值到 I2S 的 16 kHz。
// 这些 UUID 必须与固件一一对应，改固件时同步改这里。
namespace BoardProtocol {
inline constexpr const char *kServiceUuid      = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
inline constexpr const char *kControlCharUuid  = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
inline constexpr const char *kAudioCharUuid    = "66666666-6666-6666-6666-666666666666";
inline constexpr const char *kPlaybackCharUuid = "88888888-8888-8888-8888-888888888888";
// 开发板 I2S 的固定采样率（固件 SAMPLE_RATE = 16000），也是上行麦克风的采样率。
inline constexpr int kSampleRate = 16000;
// 下行（手机→板）为什么不直接发 16 kHz：Qt Android 把每次 writeCharacteristic 串行化，
// 实测持续吞吐只有约 24 KB/s，低于 16 kHz int16 所需的 32 KB/s，回传必然积压。
// 所以手机直接发 TTS 原生采样率（8 kHz = 16 KB/s），由板子升到 16 kHz。
// 固件只支持 16000 的整数约数（8000 / 4000 / 16000 ...）。
inline constexpr int kMaxDownlinkRate = 16000;
// 控制特征上的播放指令：写 "play" 后开发板才接收播放特征上的音频。
inline constexpr const char *kCmdPlay  = "play";
inline constexpr const char *kCmdStop  = "stop";
// 下行采样率指令前缀，完整指令为 "rate:8000"。必须在 play 之前发（播放中固件会拒绝）。
inline constexpr const char *kCmdRatePrefix = "rate:";
// 固件对每条指令都会在控制特征上 notify 一条文本回执：成功以 "OK:" 开头，
// 失败以 "ERROR:" 开头。"play" 的成功回执是 kReplyPlaying——手机收到它才算
// 板子真的进入了播放态（见 MainWindow::requestBoardPlay）。
inline constexpr const char *kReplyPlaying     = "OK:playing";
inline constexpr const char *kReplyErrorPrefix = "ERROR:";
} // namespace BoardProtocol

class DeviceHandler : public QObject
{
    Q_OBJECT
public:
    explicit DeviceHandler(QObject *parent = nullptr);
    ~DeviceHandler();

    void startScan();
    void stopScan();
    void connectToDevice(const QBluetoothDeviceInfo &device);
    void disconnectDevice();

    // 启用/关闭指定特征的通知，并把它设为「当前激活」的上行数据来源
    // （同一时刻只有一条激活特征，其数据经 dataReceived 送出）。
    // 返回 false 表示根本没发出去：服务不存在、特征尚未发现或没有 CCCD 描述符。
    // 返回 true 只代表开关已写出；写入本身若失败，会经 statusChanged 另行上报。
    bool enableCharacteristicNotification(const QString &serviceUuid,
                                          const QString &charUuid,
                                          bool enable = true);

    // 设置 BLE 写入目标（把 AI 回复等下行数据写回眼镜）。
    // 返回 false 表示目标无效（服务/特征不存在或不可写）。
    bool setWriteTarget(const QString &serviceUuid, const QString &charUuid);

    // 是否已发现指定 UUID 的服务。用于识别开发板并自动配置收发特征。
    bool servicePresent(const QString &uuid) const;
    // 指定服务下的特征是否已发现且有效。服务详情发现失败时服务对象仍在，
    // 但特征全是无效的——自动配置必须逐个核实，不能只看服务在不在。
    bool characteristicPresent(const QString &serviceUuid, const QString &charUuid) const;

    // ---- 控制通道（开发板的 play/stop/rate 等文本指令与回执）----
    //
    // 控制通道与下行队列（setWriteTarget / writeData / enqueueStreamData）是两条
    // 独立的路：指令很短且少，不分片、不走节拍；它的成败也单独记账、单独上报，
    // 绝不牵连音频队列（控制特征与播放特征同属一个服务，Qt 的错误回调又不带特征，
    // 所以归属靠 m_inflightWrites 的提交顺序判定，见 onServiceError）。
    //
    // setControlChannel() 绑定控制特征并开启它的通知：固件的回执只走 notify，
    // 收不到回执就无从确认指令是否生效，所以「可写 + 可通知 + 有 CCCD」缺一不可，
    // 否则返回 false。与上面的「激活特征」互不影响，两路通知可以同时开着。
    bool setControlChannel(const QString &serviceUuid, const QString &charUuid);
    void clearControlChannel();
    // 写入一条指令。返回 false 表示控制通道未建立或指令为空（什么都没发）。
    // 返回 true 只代表已提交；最终失败经 controlWriteFailed() 上报，
    // 固件的回执经 controlReply() 上报。
    bool writeControlValue(const QByteArray &data);

    // 设置流式回传音频的字节率（字节/秒 = 采样率 × 2）。默认 8 kHz（16000 字节/秒）。
    // 无响应写入(WriteWithoutResponse，开发板播放特征即此类)的定时器节拍用它；
    // 只支持有响应写入(WriteWithResponse)的特征由每包 ACK 往返天然限速，
    // 节拍器不参与，此时本值只用于换算流式队列的积压上限与水位（见 enqueueStreamData）。
    void setStreamByteRate(int bytesPerSecond);
    // 向写入目标发送数据；超长内容按当前 MTU 自动分包发送。
    // 发送结果通过 writeFinished / writeError 信号异步返回。
    void writeData(const QByteArray &data);

    // 流式下行（TTS 音频回传眼镜）：允许上一批还没发完就继续入队。
    // 与 writeData() 的差别只有「忙时不拒绝」这一点，而这一点是必需的：
    // 音频按真实时间连续产生，拒绝一次就是丢一段声音。
    // 发送节拍按 MTU 自适应到略高于音频码率（见 streamTickIntervalMs），
    // 队列积压超过上限时**停止接收**并发 streamAborted()，绝不无限增长。
    // 返回 false 表示本次数据未入队（未设置写入目标 / 数据为空 / 队列已溢出）。
    bool enqueueStreamData(const QByteArray &data);

    // 丢弃尚未发出的流式数据（关闭回传、停止朗读、断开时调用）。
    // 只清流式分片，不影响 writeData() 已排队的普通下行数据。
    void clearStreamQueue();

    // 当前排队、尚未写出的流式数据字节数。调用方据此做背压：
    // 蓝牙慢于音频产生速率时，应该暂停产生音频，而不是等队列溢出被中止。
    qint64 streamQueuedBytes() const { return m_streamQueuedBytes; }

    // 流式队列的「高水位」：超过它时调用方应暂停喂数据（约 1.5 秒音频，
    // 远低于 4 秒的溢出上限，所以背压生效时溢出保护永远不会被触发）。
    qint64 streamHighWaterBytes() const;
    // 流式队列的「低水位」：降到它以下时发 streamLowWater()，通知调用方恢复喂数据
    // （约 0.5 秒音频，足够蓝牙链路在合成下一句期间不断流）。
    qint64 streamLowWaterBytes() const;

signals:
    void deviceDiscovered(const QBluetoothDeviceInfo &info);
    void scanFinished();
    void statusChanged(const QString &status);
    void dataReceived(const QByteArray &data);
    void connected();
    void disconnected();

    // 服务与特征发现信号
    void serviceDiscovered(const QString &serviceUuid);          // 每发现一个服务发出
    void characteristicDiscovered(const QString &serviceUuid,
                                  const QString &charUuid,
                                  const QString &charName,
                                  int properties);                // 每发现一个特征发出
    void serviceDetailsDiscoveryFinished();                       // 所有服务的详情发现完毕

    // 控制通道信号
    void controlReply(const QByteArray &value);                   // 控制特征的 notify 回执（原始字节）
    void controlWriteFailed(const QString &reason);               // 一条控制指令写入失败

    // 下行写入信号
    void writeFinished();                                         // 整条数据发送完成
    void writeError(const QString &error);                        // 写入失败
    // 流式下行中止：吞吐跟不上音频产生速率（队列溢出）或写入目标报错。
    // 调用方（MainWindow）据此关闭音频回传，而不是继续往一个发不出去的目标堆数据。
    void streamAborted(const QString &reason);
    // 流式队列从高水位之上降到低水位及以下（或被清空）时发出一次，
    // 表示调用方可以恢复喂数据。只在「曾经越过高水位」后触发，不会每包都发。
    void streamLowWater();
    // 流式队列被发空（最后一个分片已写出）。朗读自然结束时据此再让开发板 stop，
    // 否则 stop 会截掉队列里尚未发出的尾音。
    void streamDrained();

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);

    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);

    void onWriteTimerTimeout();
    void onCharacteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onServiceError(QLowEnergyService::ServiceError error);

private:
    // 一个待发送分片。oneShot 标记它来自 writeData()：只有这种「整批」数据
    // 发完时才发 writeFinished；流式音频没有「一批发完」这个概念，
    // 否则每排空一次队列就会往状态栏刷一条消息。
    struct WriteChunk {
        QByteArray data;
        bool oneShot = false;
    };

    // 一次已交给 Qt、尚未收到完成/错误回调的写入属于哪条路。
    enum class WriteKind { None, Control, OneShot, Stream };
    struct InflightWrite {
        WriteKind kind = WriteKind::None;
        QLowEnergyService *service = nullptr;
    };
    enum class InflightFilter { Any, ControlOnly, DataOnly };

    // 在途写入记账（只用于错误归属与销账，不参与流控）。
    // QLowEnergyService::errorOccurred 只给错误码，不说是哪条特征；而 Qt Android
    // 把同一连接上的读写串行执行，回调顺序即提交顺序——所以「该服务上最早那条
    // 还没回调的写入」就是出错的那条。
    void noteInflight(WriteKind kind, QLowEnergyService *service);
    WriteKind takeInflight(QLowEnergyService *service, InflightFilter filter);
    void dropInflight(InflightFilter filter);

    // 连接级状态的统一复位：主动断开与意外断开走同一段，避免两处各清一半。
    void resetConnectionState();
    // 服务详情发现收尾（全部完成，或超时后带着已发现的部分继续）
    void finishDetailsDiscovery(bool timedOut);

    // 根据服务UUID查找对应的QLowEnergyService对象
    QLowEnergyService* findService(const QString &uuid) const;
    // 分包大小：由协商后的 ATT MTU 决定（MTU - 3 字节 ATT 头），未知时按默认 23 处理
    int writeChunkSize() const;
    // 无响应模式下流式发包的节拍（ms）：让发送速率略高于音频产生速率，
    // MTU 大时包大、间隔长，MTU 小时间隔短，避免固定间隔在大包时突发、小包时饥饿
    int streamTickIntervalMs() const;
    // 队列非空且发送通道空闲时立刻启动发送（否则等定时器/完成回调推进）
    void startSendingIfIdle();
    // 队列排空后的收尾：只对普通下行发 writeFinished
    void finishOneShotWriteIfDone();
    // 发送队首分片；队列空时收尾并发出 writeFinished
    void sendNextWriteChunk();
    // 清空待发送队列并停止发包定时器
    void clearWriteQueue();
    // 根据当前队列水位更新 m_streamAboveHighWater，必要时发 streamLowWater
    void updateStreamWatermarks();

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_controller = nullptr;
    QList<QLowEnergyService*> m_services;     // 所有已创建的服务对象
    bool m_servicesDiscovered = false;        // 是否已完成服务发现（控制器级别）
    QSet<QLowEnergyService*> m_detailsPending; // 详情发现尚未有结果的服务
    QTimer *m_detailsTimer = nullptr;         // 详情发现的总超时（某个服务卡住时兜底）

    // 当前激活的特征（用于数据接收）
    QLowEnergyService *m_activeService = nullptr;
    QLowEnergyCharacteristic m_activeCharacteristic;

    // 控制通道（见 setControlChannel）
    QLowEnergyService *m_controlService = nullptr;
    QLowEnergyCharacteristic m_controlCharacteristic;
    QList<InflightWrite> m_inflightWrites;     // 在途写入，按提交顺序

    // 写入目标（AI 回复等下行数据）
    QLowEnergyService *m_writeService = nullptr;
    QLowEnergyCharacteristic m_writeCharacteristic;
    QList<WriteChunk> m_writeQueue;            // 待发送的分片队列
    QTimer *m_writeTimer = nullptr;            // 无响应模式下的发包节拍
    QLowEnergyService::WriteMode m_writeMode = QLowEnergyService::WriteWithoutResponse;
    int m_streamByteRate = 8000 * 2;           // 流式回传字节率（默认 8 kHz int16）
    qint64 m_streamQueuedBytes = 0;            // 队列中流式数据的字节数（用于积压上限）
    bool m_streamOverflowed = false;           // 已溢出：队列排空前不再接收流式数据
    bool m_streamAboveHighWater = false;       // 曾越过高水位，等待降回低水位后发 streamLowWater
    bool m_streamWasActive = false;            // 有流式数据入过队，排空后发 streamDrained

    // 吞吐诊断：统计窗口内实际交给蓝牙栈的流式字节数，每秒打印一次。
    // 用来判断瓶颈是「连接间隔/MTU 太低」还是别处——见 logStreamThroughput()。
    void logStreamThroughput(int bytesJustSent);
    QElapsedTimer m_statTimer;
    QElapsedTimer m_statLastSend;   // 上一次出队时刻，用于识别空闲间隙
    qint64 m_statBytes = 0;
    int m_statChunks = 0;
    bool m_oneShotPending = false;             // 队列中有普通下行数据待收尾
    bool m_writeInFlight = false;              // 有响应写入：上一包未确认前不能再发
    // 无响应写入的闭环流控：已交给 Qt、尚未收到 characteristicWritten 的分片数。
    // Qt Android 在 Java 侧有一条无上限的 IO 队列，writeCharacteristic 立即返回；
    // 若只按定时器开环发包，发得比 Android 栈消费得快，差额会在这条队列里无限堆积，
    // App 一侧完全看不见（延迟越攒越大，停止后还要播完）。用完成回调约束在途数即可闭环。
    int m_inflightChunks = 0;
    QElapsedTimer m_lastAckTimer;              // 上次收到 characteristicWritten 的时刻（防死锁）
    // 连续写入错误计数：偶发单次错误（Qt 3 秒 IO 超时等）不应中止整个回传
    int m_consecutiveWriteErrors = 0;
};

#endif // DEVICEHANDLER_H