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

    // 原有信号连接
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

    connect(m_deviceHandler, &DeviceHandler::serviceDiscovered, this,
            [this](const QString &uuid) {
                QTreeWidgetItem *serviceItem = new QTreeWidgetItem(m_serviceTree);
                serviceItem->setText(0, uuid);
                serviceItem->setData(0, Qt::UserRole, "service");
                serviceItem->setData(0, Qt::UserRole + 1, uuid);
                m_serviceTree->addTopLevelItem(serviceItem);
            });

    connect(m_deviceHandler, &DeviceHandler::characteristicDiscovered, this,
            [this](const QString &serviceUuid, const QString &charUuid, const QString &name, int props) {
                for (int i = 0; i < m_serviceTree->topLevelItemCount(); ++i) {
                    QTreeWidgetItem *serviceItem = m_serviceTree->topLevelItem(i);
                    if (serviceItem->data(0, Qt::UserRole + 1).toString() == serviceUuid) {
                        QTreeWidgetItem *charItem = new QTreeWidgetItem(serviceItem);
                        QString displayName = name.isEmpty() ? charUuid : name + " (" + charUuid + ")";
                        charItem->setText(0, displayName);
                        QStringList propsList;
                        if (props & QLowEnergyCharacteristic::Read) propsList << "Read";
                        if (props & QLowEnergyCharacteristic::Write) propsList << "Write";
                        if (props & QLowEnergyCharacteristic::Notify) propsList << "Notify";
                        if (props & QLowEnergyCharacteristic::Indicate) propsList << "Indicate";
                        charItem->setText(1, propsList.join(", "));
                        charItem->setData(0, Qt::UserRole, "characteristic");
                        charItem->setData(0, Qt::UserRole + 1, serviceUuid);
                        charItem->setData(0, Qt::UserRole + 2, charUuid);
                        serviceItem->addChild(charItem);
                        serviceItem->setExpanded(true);
                        break;
                    }
                }
            });

    connect(m_serviceTree, &QTreeWidget::itemClicked, this, &MainWindow::onServiceTreeItemClicked);

    // ========== 新增：初始化 Whisper ==========
    m_whisperManager = new WhisperManager(this);
    connect(m_whisperManager, &WhisperManager::transcriptionReady,
            this, &MainWindow::onTranscriptionResult);
    connect(m_whisperManager, &WhisperManager::errorOccurred,
            this, &MainWindow::onWhisperError);

    QString modelPath = "/sdcard/ggml-base-q5_1.bin";
    if (!m_whisperManager->init(modelPath)) {
        appendStatus("Whisper 模型加载失败，请检查文件是否存在！");
    } else {
        appendStatus("Whisper 模型加载成功！");
    }

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

// ========== 修改后的 onDataReceived ==========
void MainWindow::onDataReceived(const QByteArray &data)
{
    // 十六进制显示（调试用）
    QString hexString;
    for (int i = 0; i < data.size(); ++i) {
        hexString += QString("%1 ").arg((quint8)data.at(i), 2, 16, QLatin1Char('0')).toUpper();
    }
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_dataDisplay->append(QString("[%1] %2").arg(timeStr, hexString.trimmed()));

    // 新增：累积音频数据并喂给 Whisper
    m_audioBuffer.append(data);
    const int CHUNK_SIZE_BYTES = 3 * 16000 * 2;  // 96000 字节
    if (m_audioBuffer.size() >= CHUNK_SIZE_BYTES && m_whisperManager) {
        const int16_t *pcmData = reinterpret_cast<const int16_t*>(m_audioBuffer.constData());
        size_t sampleCount = m_audioBuffer.size() / 2;
        m_whisperManager->feedAudioData(pcmData, sampleCount);
        m_audioBuffer.clear();
    }
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
        QString props = item->text(1);
        if (props.contains("Notify") || props.contains("Indicate")) {
            m_deviceHandler->enableCharacteristicNotification(serviceUuid, charUuid, true);
            appendStatus("Enabling notification on: " + charUuid);
        } else {
            appendStatus("Characteristic does not support Notify/Indicate.");
        }
    }
}

// ========== 新增：Whisper 结果回调 ==========
void MainWindow::onTranscriptionResult(const QString &text)
{
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_dataDisplay->append(QString("[识别] %1").arg(text));
}

void MainWindow::onWhisperError(const QString &error)
{
    appendStatus("Whisper 错误: " + error);
}