#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QBluetoothUuid>
#include <QMap>

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

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);

    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);

private:
    // 根据服务UUID查找对应的QLowEnergyService对象
    QLowEnergyService* findService(const QString &uuid) const;

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QLowEnergyController *m_controller = nullptr;
    QList<QLowEnergyService*> m_services;     // 所有已创建的服务对象
    bool m_servicesDiscovered = false;        // 是否已完成服务发现（控制器级别）
    int m_pendingServiceDetails = 0;          // 正在等待 detail 发现的服务数量

    // 当前激活的特征（用于数据接收）
    QLowEnergyService *m_activeService = nullptr;
    QLowEnergyCharacteristic m_activeCharacteristic;
};

#endif // DEVICEHANDLER_H