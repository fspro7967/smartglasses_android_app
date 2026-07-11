#include "mainwindow.h"
#include <QVBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QPermissions>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_deviceHandler(new DeviceHandler(this))
{
    setWindowTitle("Bluetooth Hex Viewer");

    // ========== 创建所有子控件 ==========
    m_scanButton = new QPushButton("Start Scan");
    m_deviceList = new QListWidget();
    m_serviceTree = new QTreeWidget();
    m_serviceTree->setHeaderLabels({"Service / Characteristic", "Properties"});
    m_serviceTree->header()->setStretchLastSection(true);

    m_dataDisplay = new QTextEdit();
    m_dataDisplay->setReadOnly(true);

    m_recStartBtn = new QPushButton("开始录音");
    m_recStopBtn = new QPushButton("停止录音");
    m_recStopBtn->setEnabled(false);
    m_recStatusLabel = new QLabel("录音状态：空闲");

    // ========== 连接选项卡（扫描 + 服务） ==========
    m_connectTab = new QTabWidget();
    QWidget *scanPage = new QWidget();
    QVBoxLayout *scanLayout = new QVBoxLayout(scanPage);
    scanLayout->addWidget(m_scanButton);
    scanLayout->addWidget(m_deviceList);
    m_connectTab->addTab(scanPage, "扫描设备");

    QWidget *servicePage = new QWidget();
    QVBoxLayout *serviceLayout = new QVBoxLayout(servicePage);
    serviceLayout->addWidget(m_serviceTree);
    m_connectTab->addTab(servicePage, "服务/特征");

    // ========== 数据选项卡（实时数据 + 录音） ==========
    m_dataTab = new QTabWidget();
    m_dataTab->addTab(m_dataDisplay, "实时数据");

    QWidget *recordPage = new QWidget();
    QVBoxLayout *recordLayout = new QVBoxLayout(recordPage);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_recStartBtn);
    btnLayout->addWidget(m_recStopBtn);
    recordLayout->addLayout(btnLayout);
    recordLayout->addWidget(m_recStatusLabel);
    recordLayout->addStretch();
    m_dataTab->addTab(recordPage, "音频录制");

    // ========== 主选项卡 ==========
    m_mainTabWidget = new QTabWidget();
    m_mainTabWidget->addTab(m_connectTab, "连接");
    m_mainTabWidget->addTab(m_dataTab, "数据");
    setCentralWidget(m_mainTabWidget);

    // ========== 信号连接（保持不变） ==========
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

    // 服务树更新信号（方案2原有代码，保留不变）
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

    // 录音按钮连接
    connect(m_recStartBtn, &QPushButton::clicked, this, &MainWindow::onStartRecording);
    connect(m_recStopBtn, &QPushButton::clicked, this, &MainWindow::onStopRecording);

    // 权限请求
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

    // 如果正在录音，缓存数据
    if (m_isRecording) {
        m_recBuffer.append(data);
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

void MainWindow::onStartRecording()
{
    if (m_isRecording) return;
    m_recBuffer.clear();
    m_isRecording = true;
    m_recStartBtn->setEnabled(false);
    m_recStopBtn->setEnabled(true);
    m_recStatusLabel->setText("录音状态：录音中...");
    appendStatus("开始录音");
}

void MainWindow::onStopRecording()
{
    if (!m_isRecording) return;
    m_isRecording = false;
    m_recStartBtn->setEnabled(true);
    m_recStopBtn->setEnabled(false);
    m_recStatusLabel->setText("录音状态：空闲");
    appendStatus(QString("停止录音，共 %1 字节").arg(m_recBuffer.size()));

    if (m_recBuffer.isEmpty()) return;

    // 保存原始数据到文件（这里先保存为 .pcm 或 .mp3，根据实际编码调整）
    QString filePath = saveAudioToFile(m_recBuffer);
    if (!filePath.isEmpty()) {
        appendStatus("文件已保存: " + filePath);
    } else {
        appendStatus("文件保存失败！");
    }
}

QString MainWindow::saveAudioToFile(const QByteArray &audioData)
{
    // 生成保存路径
    QString dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QDir().mkpath(dir);
    QString fileName = QString("rec_%1.mp3")
                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = dir + "/" + fileName;

    // ========== 编码为 MP3（需自行实现） ==========
    // 方式1：如果设备直接发送的就是 MP3 帧，直接写入即可
    // QFile file(filePath);
    // if (file.open(QIODevice::WriteOnly)) {
    //     file.write(audioData);
    //     file.close();
    //     return filePath;
    // }

    // 方式2：如果原始数据是 PCM，需要转码为 MP3。
    // 以下提供一个调用 LAME 的伪代码示例（需集成 libmp3lame）：
    /*
    #include <lame/lame.h>
    // 假设音频参数：采样率 8000，单声道，16bit
    lame_t lame = lame_init();
    lame_set_in_samplerate(lame, 8000);
    lame_set_num_channels(lame, 1);
    lame_set_VBR(lame, vbr_default);
    lame_init_params(lame);

    QByteArray mp3buf(audioData.size() * 1.25 + 7200, 0); // 预留空间
    int mp3size = lame_encode_buffer_interleaved(
        lame,
        reinterpret_cast<short*>(audioData.data()),
        audioData.size() / 2,
        reinterpret_cast<unsigned char*>(mp3buf.data()),
        mp3buf.size()
    );
    mp3size += lame_encode_flush(lame, reinterpret_cast<unsigned char*>(mp3buf.data() + mp3size), mp3buf.size() - mp3size);
    lame_close(lame);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(mp3buf.left(mp3size));
        file.close();
        return filePath;
    }
    */

    // 方式3：通过 JNI 调用 Android MediaCodec 编码（略）

    // 临时：直接保存原始数据（后缀 .raw），便于调试
    QString rawPath = dir + "/" + fileName + ".raw";
    QFile file(rawPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(audioData);
        file.close();
        return rawPath;
    }

    return QString();
}