#include "mainwindow.h"
#include <QVBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QPermissions>
#include <QTreeWidgetItem>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_deviceHandler(new DeviceHandler(this))
    , m_deviceList(new QListWidget(this))
    , m_scanButton(new QPushButton("Start Scan", this))
    , m_dataDisplay(new QTextEdit(this))
    , m_serviceTree(new QTreeWidget(this))
{
    setWindowTitle("Bluetooth Hex Viewer");

    // 布局
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(m_scanButton);
    layout->addWidget(m_deviceList);
    layout->addWidget(m_serviceTree);
    layout->addWidget(m_dataDisplay);
    setCentralWidget(centralWidget);

    m_dataDisplay->setReadOnly(true);
    m_serviceTree->setHeaderLabels({"Service / Characteristic", "Properties"});
    m_serviceTree->header()->setStretchLastSection(true);

    // 连接信号
    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_deviceList, &QListWidget::itemClicked, this, &MainWindow::onDeviceSelected);
    connect(m_deviceHandler, &DeviceHandler::dataReceived, this, &MainWindow::onDataReceived);
    connect(m_deviceHandler, &DeviceHandler::statusChanged, this, &MainWindow::appendStatus);
    connect(m_deviceHandler, &DeviceHandler::deviceDiscovered, this, [this](const QBluetoothDeviceInfo &info) {
        m_discoveredDevices.append(info);
        m_deviceList->addItem(info.name().isEmpty() ? info.address().toString() : info.name());
    });
    connect(m_deviceHandler, &DeviceHandler::connected, this, [this]() {
        m_serviceTree->clear();
        appendStatus("Connected.");
    });
    connect(m_deviceHandler, &DeviceHandler::disconnected, this, [this]() {
        m_serviceTree->clear();
        appendStatus("Disconnected.");
    });

    // 服务与特征发现 -> 更新树
    connect(m_deviceHandler, &DeviceHandler::serviceDiscovered, this,
            [this](const QString &uuid) {
                // 创建服务根节点
                QTreeWidgetItem *serviceItem = new QTreeWidgetItem(m_serviceTree);
                serviceItem->setText(0, uuid);
                serviceItem->setData(0, Qt::UserRole, "service");
                serviceItem->setData(0, Qt::UserRole + 1, uuid);
                m_serviceTree->addTopLevelItem(serviceItem);
            });

    connect(m_deviceHandler, &DeviceHandler::characteristicDiscovered, this,
            [this](const QString &serviceUuid, const QString &charUuid, const QString &name, int props) {
                // 查找对应的服务节点
                for (int i = 0; i < m_serviceTree->topLevelItemCount(); ++i) {
                    QTreeWidgetItem *serviceItem = m_serviceTree->topLevelItem(i);
                    if (serviceItem->data(0, Qt::UserRole + 1).toString() == serviceUuid) {
                        QTreeWidgetItem *charItem = new QTreeWidgetItem(serviceItem);
                        QString displayName = name.isEmpty() ? charUuid : name + " (" + charUuid + ")";
                        charItem->setText(0, displayName);
                        // 构建属性字符串
                        QStringList propsList;
                        if (props & QLowEnergyCharacteristic::Read) propsList << "Read";
                        if (props & QLowEnergyCharacteristic::Write) propsList << "Write";
                        if (props & QLowEnergyCharacteristic::Notify) propsList << "Notify";
                        if (props & QLowEnergyCharacteristic::Indicate) propsList << "Indicate";
                        charItem->setText(1, propsList.join(", "));
                        charItem->setData(0, Qt::UserRole, "characteristic");
                        charItem->setData(0, Qt::UserRole + 1, serviceUuid);   // 服务UUID
                        charItem->setData(0, Qt::UserRole + 2, charUuid);      // 特征UUID
                        serviceItem->addChild(charItem);
                        serviceItem->setExpanded(true);
                        break;
                    }
                }
            });

    connect(m_serviceTree, &QTreeWidget::itemClicked, this, &MainWindow::onServiceTreeItemClicked);

    requestAndroidPermissions();
}

MainWindow::~MainWindow() {}

void MainWindow::onScanClicked()
{
    m_deviceList->clear();
    m_discoveredDevices.clear();
    m_deviceHandler->startScan();
}

void MainWindow::onDeviceSelected(QListWidgetItem *item)
{
    int index = m_deviceList->row(item);
    if (index >= 0 && index < m_discoveredDevices.size()) {
        m_deviceHandler->connectToDevice(m_discoveredDevices.at(index));
    }
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    QString hexString;
    for (int i = 0; i < data.size(); ++i) {
        hexString += QString("%1 ").arg((quint8)data.at(i), 2, 16, QLatin1Char('0')).toUpper();
    }
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_dataDisplay->append(QString("[%1] %2").arg(timeStr, hexString.trimmed()));
}

void MainWindow::appendStatus(const QString &msg)
{
    m_dataDisplay->append("--- " + msg + " ---");
}

void MainWindow::onBluetoothPermissionGranted()
{
    qDebug() << "Bluetooth permission granted.";
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Approximate);
    qApp->requestPermission(locationPermission, this, [this](const QPermission &perm) {
        if (perm.status() == Qt::PermissionStatus::Granted) {
            qDebug() << "Location permission granted.";
        } else {
            qWarning() << "Location permission denied.";
        }
    });
}

void MainWindow::requestAndroidPermissions()
{
    QBluetoothPermission bluetoothPermission;
    bluetoothPermission.setCommunicationModes(QBluetoothPermission::Access);

    QApplication *app = qApp;
    if (!app) return;

    app->requestPermission(bluetoothPermission, this, &MainWindow::onBluetoothPermissionGranted);
}

void MainWindow::onServiceTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    QString type = item->data(0, Qt::UserRole).toString();
    if (type == "characteristic") {
        QString serviceUuid = item->data(0, Qt::UserRole + 1).toString();
        QString charUuid = item->data(0, Qt::UserRole + 2).toString();
        // 只对支持 Notify 或 Indicate 的特征启用通知
        QString props = item->text(1);
        if (props.contains("Notify") || props.contains("Indicate")) {
            m_deviceHandler->enableCharacteristicNotification(serviceUuid, charUuid, true);
            appendStatus("Enabling notification on: " + charUuid);
        } else {
            appendStatus("Characteristic does not support Notify/Indicate.");
        }
    }
}