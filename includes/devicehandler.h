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
    // 向写入目标发送数据；超长内容按当前 MTU 自动分包发送。
    // 发送结果通过 writeFinished / writeError 信号异步返回。
    void writeData(const QByteArray &data);

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
    // 根据服务UUID查找对应的QLowEnergyService对象
    QLowEnergyService* findService(const QString &uuid) const;
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
    QList<QByteArray> m_writeQueue;            // 待发送的分片队列
    QTimer *m_writeTimer = nullptr;            // 无响应模式下的发包节拍
    QLowEnergyService::WriteMode m_writeMode = QLowEnergyService::WriteWithoutResponse;
};

#endif // DEVICEHANDLER_H