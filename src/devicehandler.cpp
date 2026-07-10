// devicehandler.cpp (关键部分)
#include "devicehandler.h"
#include <QDebug>

DeviceHandler::DeviceHandler(QObject *parent) : QObject(parent)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &DeviceHandler::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &DeviceHandler::onScanError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &DeviceHandler::scanFinished);
}

void DeviceHandler::startScan()
{
    m_discoveryAgent->start();
    emit statusChanged("Scanning...");
}

void DeviceHandler::connectToDevice(const QBluetoothDeviceInfo &device)
{
    if (m_controller) {
        m_controller->disconnectFromDevice();
        delete m_controller;
    }
    m_controller = QLowEnergyController::createCentral(device, this);
    connect(m_controller, &QLowEnergyController::connected, this, [this]() {
        emit statusChanged("Connected. Discovering services...");
        m_controller->discoverServices();
    });
    connect(m_controller, &QLowEnergyController::disconnected, this, &DeviceHandler::disconnected);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &DeviceHandler::onServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished,
            this, &DeviceHandler::onServiceDiscoveryFinished);
    m_controller->connectToDevice();
}

void DeviceHandler::onServiceDiscovered(const QBluetoothUuid &newService)
{
    // 确认是不是我们需要的服务
    if (newService == m_serviceUuid) {
        m_service = m_controller->createServiceObject(newService, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &DeviceHandler::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &DeviceHandler::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void DeviceHandler::onServiceStateChanged(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered) {
        // 查找目标特征并启用通知
        m_targetCharacteristic = m_service->characteristic(m_characteristicUuid);
        if (m_targetCharacteristic.isValid()) {
            QLowEnergyDescriptor notification = m_targetCharacteristic.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (notification.isValid()) {
                m_service->writeDescriptor(notification, QByteArray::fromHex("0100")); // 启用通知
                emit statusChanged("Notification enabled. Ready to receive data.");
            }
        }
    }
}

void DeviceHandler::onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() == m_characteristicUuid) {
        emit dataReceived(value);   // 原始数据直接发出
    }
}