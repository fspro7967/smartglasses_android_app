#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QBluetoothUuid>
#include <QMap>

class QTimer;

// 开发板（ESP32-C3 蓝牙音频收发器，见 smartglasses_board/src/main.cpp）的协议常量。
// 开发板与手机各用一条特征：控制特征收发文本指令，音频特征上行麦克风 PCM，
// 播放特征下行扬声器 PCM。三者采样率固定 16 kHz / 单声道 / int16 小端。
// 这些 UUID 必须与固件一一对应，改固件时同步改这里。
namespace BoardProtocol {
inline constexpr const char *kServiceUuid      = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
inline constexpr const char *kControlCharUuid  = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
inline constexpr const char *kAudioCharUuid    = "66666666-6666-6666-6666-666666666666";
inline constexpr const char *kPlaybackCharUuid = "88888888-8888-8888-8888-888888888888";
// 开发板扬声器回传 PCM 的采样率（固件 SAMPLE_RATE = 16000）。
inline constexpr int kSampleRate = 16000;
// 控制特征上的播放指令：写 "play" 后开发板才接收播放特征上的音频。
inline constexpr const char *kCmdPlay  = "play";
inline constexpr const char *kCmdStop  = "stop";
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

    // 启用指定特征的通知/指示
    void enableCharacteristicNotification(const QString &serviceUuid,
                                          const QString &charUuid,
                                          bool enable = true);

    // 设置 BLE 写入目标（把 AI 回复等下行数据写回眼镜）。
    // 返回 false 表示目标无效（服务/特征不存在或不可写）。
    bool setWriteTarget(const QString &serviceUuid, const QString &charUuid);

    // 是否已发现指定 UUID 的服务。用于识别开发板并自动配置收发特征。
    bool servicePresent(const QString &uuid) const;

    // 向「控制特征」写入一条短指令（开发板的 play/stop/volume/status 等）。
    // 与 setWriteTarget 的下行队列相互独立：控制特征与音频播放特征不是同一条，
    // 指令也不该和音频分片混在一个队列里排队。指令很短且少，这里直接写入，
    // 不做分片、不做节拍控制。
    // 返回 false 表示服务/特征不存在或不可写。
    bool writeControlValue(const QString &serviceUuid,
                           const QString &charUuid,
                           const QByteArray &data);

    // 设置流式回传音频的字节率（字节/秒 = 采样率 × 2）。默认 8 kHz（16000 字节/秒）。
    // 仅无响应写入(WriteWithoutResponse)的定时器节拍用它；有响应写入
    // (WriteWithResponse，开发板播放特征即此类) 由每包 ACK 往返天然限速，
    // 节拍器不参与，此时本值只用于换算流式队列的积压上限（见 enqueueStreamData）。
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

    // 下行写入信号
    void writeFinished();                                         // 整条数据发送完成
    void writeError(const QString &error);                        // 写入失败
    // 流式下行中止：吞吐跟不上音频产生速率（队列溢出）或写入目标报错。
    // 调用方（MainWindow）据此关闭音频回传，而不是继续往一个发不出去的目标堆数据。
    void streamAborted(const QString &reason);

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

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_controller = nullptr;
    QList<QLowEnergyService*> m_services;     // 所有已创建的服务对象
    bool m_servicesDiscovered = false;        // 是否已完成服务发现（控制器级别）
    int m_pendingServiceDetails = 0;          // 正在等待 detail 发现的服务数量

    // 当前激活的特征（用于数据接收）
    QLowEnergyService *m_activeService = nullptr;
    QLowEnergyCharacteristic m_activeCharacteristic;

    // 写入目标（AI 回复等下行数据）
    QLowEnergyService *m_writeService = nullptr;
    QLowEnergyCharacteristic m_writeCharacteristic;
    QList<WriteChunk> m_writeQueue;            // 待发送的分片队列
    QTimer *m_writeTimer = nullptr;            // 无响应模式下的发包节拍
    QLowEnergyService::WriteMode m_writeMode = QLowEnergyService::WriteWithoutResponse;
    int m_streamByteRate = 8000 * 2;           // 流式回传字节率（默认 8 kHz int16）
    qint64 m_streamQueuedBytes = 0;            // 队列中流式数据的字节数（用于积压上限）
    bool m_streamOverflowed = false;           // 已溢出：队列排空前不再接收流式数据
    bool m_oneShotPending = false;             // 队列中有普通下行数据待收尾
    bool m_writeInFlight = false;              // 有响应写入：上一包未确认前不能再发
};

#endif // DEVICEHANDLER_H