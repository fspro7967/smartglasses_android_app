#include "devicehandler.h"
#include <QDebug>

DeviceHandler::DeviceHandler(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
    , m_controller(nullptr)
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &DeviceHandler::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &DeviceHandler::onScanError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &DeviceHandler::scanFinished);
}

DeviceHandler::~DeviceHandler()
{
    stopScan();
    disconnectDevice();
}

void DeviceHandler::startScan()
{
    if (m_discoveryAgent->isActive())
        m_discoveryAgent->stop();
    m_discoveryAgent->start();
    emit statusChanged("Scanning started...");
}

void DeviceHandler::stopScan()
{
    if (m_discoveryAgent->isActive()) {
        m_discoveryAgent->stop();
        emit statusChanged("Scanning stopped.");
    }
}

void DeviceHandler::connectToDevice(const QBluetoothDeviceInfo &device)
{
    disconnectDevice();

    m_controller = QLowEnergyController::createCentral(device, this);
    if (!m_controller) {
        emit statusChanged("Failed to create BLE controller.");
        return;
    }

    connect(m_controller, &QLowEnergyController::connected, this, [this]() {
        emit statusChanged("Connected. Discovering services...");
        emit connected();
        m_controller->discoverServices();
    });
    connect(m_controller, &QLowEnergyController::disconnected, this, [this]() {
        emit statusChanged("Disconnected.");
        emit disconnected();
    });
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, [this](QLowEnergyController::Error) {
                emit statusChanged("Controller error: " + m_controller->errorString());
            });
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &DeviceHandler::onServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished,
            this, &DeviceHandler::onServiceDiscoveryFinished);

    m_controller->connectToDevice();
}

void DeviceHandler::disconnectDevice()
{
    // 关闭激活的特征通知
    if (m_activeService && m_activeCharacteristic.isValid()) {
        enableCharacteristicNotification(m_activeService->serviceUuid().toString(),
                                         m_activeCharacteristic.uuid().toString(),
                                         false);
    }
    m_activeService = nullptr;
    m_activeCharacteristic = QLowEnergyCharacteristic();

    // 删除所有服务对象
    qDeleteAll(m_services);
    m_services.clear();
    m_pendingServiceDetails = 0;

    if (m_controller) {
        m_controller->disconnectFromDevice();
        delete m_controller;
        m_controller = nullptr;
    }
}

void DeviceHandler::enableCharacteristicNotification(const QString &serviceUuid,
                                                     const QString &charUuid,
                                                     bool enable)
{
    QLowEnergyService *service = findService(serviceUuid);
    if (!service) {
        qWarning() << "Service not found:" << serviceUuid;
        return;
    }

    QLowEnergyCharacteristic characteristic = service->characteristic(QBluetoothUuid(charUuid));
    if (!characteristic.isValid()) {
        qWarning() << "Characteristic not found:" << charUuid;
        return;
    }

    QLowEnergyDescriptor cccd = characteristic.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (!cccd.isValid()) {
        qWarning() << "CCCD not found for characteristic:" << charUuid;
        return;
    }

    if (enable) {
        // 如果之前有激活的特征，先关闭它的通知
        if (m_activeService && m_activeCharacteristic.isValid()) {
            enableCharacteristicNotification(m_activeService->serviceUuid().toString(),
                                             m_activeCharacteristic.uuid().toString(),
                                             false);
        }
        service->writeDescriptor(cccd, QByteArray::fromHex("0100"));
        m_activeService = service;
        m_activeCharacteristic = characteristic;
        emit statusChanged("Notification enabled for characteristic: " + charUuid);
    } else {
        service->writeDescriptor(cccd, QByteArray::fromHex("0000"));
        if (m_activeService == service && m_activeCharacteristic.uuid() == QBluetoothUuid(charUuid)) {
            m_activeService = nullptr;
            m_activeCharacteristic = QLowEnergyCharacteristic();
        }
        emit statusChanged("Notification disabled for characteristic: " + charUuid);
    }
}

// ---------- 扫描槽 ----------
void DeviceHandler::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    emit deviceDiscovered(info);
}

void DeviceHandler::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error);
    emit statusChanged("Scan error: " + m_discoveryAgent->errorString());
}

// ---------- 服务发现 ----------
void DeviceHandler::onServiceDiscovered(const QBluetoothUuid &newService)
{
    // 为每个发现的服务创建 QLowEnergyService 对象
    QLowEnergyService *service = m_controller->createServiceObject(newService, this);
    if (!service) {
        qWarning() << "Failed to create service object for:" << newService.toString();
        return;
    }

    m_services.append(service);
    m_pendingServiceDetails++;

    connect(service, &QLowEnergyService::stateChanged,
            this, &DeviceHandler::onServiceStateChanged);
    connect(service, &QLowEnergyService::characteristicChanged,
            this, &DeviceHandler::onCharacteristicChanged);

    service->discoverDetails();
    emit serviceDiscovered(newService.toString());
}

void DeviceHandler::onServiceDiscoveryFinished()
{
    m_servicesDiscovered = true;
    emit statusChanged("Service discovery finished. Discovering details...");
    // 如果没有发现任何服务，直接通知UI
    if (m_services.isEmpty()) {
        emit serviceDetailsDiscoveryFinished();
    }
}

// ---------- 服务详情（特征）发现 ----------
void DeviceHandler::onServiceStateChanged(QLowEnergyService::ServiceState newState)
{
    QLowEnergyService *service = qobject_cast<QLowEnergyService*>(sender());
    if (!service)
        return;

    if (newState == QLowEnergyService::RemoteServiceDiscovered) {
        const QList<QLowEnergyCharacteristic> chars = service->characteristics();
        for (const QLowEnergyCharacteristic &ch : chars) {
            QString name = ch.name().isEmpty() ? ch.uuid().toString() : ch.name();
            emit characteristicDiscovered(service->serviceUuid().toString(),
                                          ch.uuid().toString(),
                                          name,
                                          (int)ch.properties());
        }
    }

    // 检查是否所有服务的详情都已发现
    if (newState == QLowEnergyService::RemoteServiceDiscovered ||
        newState == QLowEnergyService::RemoteService) {
        m_pendingServiceDetails--;
        if (m_pendingServiceDetails <= 0 && m_servicesDiscovered) {
            emit serviceDetailsDiscoveryFinished();
            emit statusChanged("All service details discovered.");
        }
    }
}

// ---------- 数据接收 ----------
void DeviceHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    // 只转发当前激活的特征数据
    if (m_activeService && m_activeCharacteristic.uuid() == c.uuid()) {
        emit dataReceived(value);
    }
}

// ---------- 辅助函数 ----------
QLowEnergyService* DeviceHandler::findService(const QString &uuid) const
{
    for (QLowEnergyService *s : m_services) {
        if (s->serviceUuid().toString() == uuid)
            return s;
    }
    return nullptr;
}