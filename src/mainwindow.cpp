#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QPermissions>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_deviceHandler(new DeviceHandler(this))
    , m_deviceList(new QListWidget(this))
    , m_scanButton(new QPushButton("Start Scan", this))
    , m_selectAudioButton(new QPushButton("Select Audio", this))
    , m_dataDisplay(new QTextEdit(this))
    , m_transcriptionDisplay(new QTextEdit(this))
    , m_serviceTree(new QTreeWidget(this))
{
    setWindowTitle("Bluetooth Hex Viewer");

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_scanButton);
    buttonLayout->addWidget(m_selectAudioButton);
    layout->addLayout(buttonLayout);
    layout->addWidget(m_deviceList);
    layout->addWidget(m_serviceTree);
    layout->addWidget(m_transcriptionDisplay);
    layout->addWidget(m_dataDisplay);
    setCentralWidget(centralWidget);

    m_dataDisplay->setReadOnly(true);
    m_transcriptionDisplay->setReadOnly(true);
    m_transcriptionDisplay->setPlaceholderText("识别文字将显示在这里...");
    QFont font = m_transcriptionDisplay->font();
    font.setPointSize(14);
    m_transcriptionDisplay->setFont(font);
    m_transcriptionDisplay->setMaximumHeight(120);
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
    connect(m_selectAudioButton, &QPushButton::clicked, this, &MainWindow::onSelectAudioFileClicked);

    // ========== 新增：初始化 Whisper ==========
    // 解压模型文件
    m_modelPath = extractModelToFile();
    if (!m_modelPath.isEmpty()) {
        appendStatus("模型文件就绪: " + m_modelPath);
    } else {
        appendStatus("模型文件提取失败！");
    }

    m_whisperManager = new WhisperManager(this);
    connect(m_whisperManager, &WhisperManager::transcriptionReady,
            this, &MainWindow::onTranscriptionResult, Qt::QueuedConnection);
    connect(m_whisperManager, &WhisperManager::errorOccurred,
            this, &MainWindow::onWhisperError, Qt::QueuedConnection);

    //QString modelPath = "/storage/emulated/0/Download/ggml-base-q5_1.bin";
    if (!m_whisperManager->init(m_modelPath)) {
        appendStatus("Whisper 模型加载失败，请检查文件！");
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

void MainWindow::onDataReceived(const QByteArray &data)
{
    // 十六进制显示（调试用）
    /*QString hexString;
    for (int i = 0; i < data.size(); ++i) {
        hexString += QString("%1 ").arg((quint8)data.at(i), 2, 16, QLatin1Char('0')).toUpper();
    }
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_dataDisplay->append(QString("[%1] %2").arg(timeStr, hexString.trimmed()));
    qInfo() << hexString;*/
    // 把收到的数据存入缓存区
    m_audioBuffer.append(data);
    //appendStatus(QString("收到 BLE 数据 %1 字节，当前缓冲区 %2 字节").arg(data.size()).arg(m_audioBuffer.size()));
    // 把缓存的数据传入模型，并清空缓存
    constexpr int CHUNK_SIZE_BYTES = 3 * 16000 * sizeof(int16_t); // 约 3 秒的 16kHz/16bit 音频
    if (m_audioBuffer.size() >= CHUNK_SIZE_BYTES && m_whisperManager) {
        const int16_t *pcmData = reinterpret_cast<const int16_t*>(m_audioBuffer.constData());
        const size_t sampleCount = static_cast<size_t>(m_audioBuffer.size() / sizeof(int16_t));
        //qDebug() << "Feed audio to Whisper:" << sampleCount << "samples from" << m_audioBuffer.size() << "bytes";
        appendStatus(QString("已将 %1 字节音频送入 Whisper，样本数 %2").arg(m_audioBuffer.size()).arg(sampleCount));
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
    appendStatus(QString("Whisper 回调收到文本: %1").arg(text));
    m_transcriptionDisplay->append(QString("[%1] %2").arg(timeStr, text));
}

void MainWindow::onWhisperError(const QString &error)
{
    appendStatus("Whisper 错误: " + error);
}

QString MainWindow::extractModelToFile()
{
    // 目标路径：应用私有目录
    QString targetDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(targetDir);
    QString targetPath = targetDir + "/model.bin";

    // 如果已经解压过，直接返回
    if (QFile::exists(targetPath)) {
        qDebug() << "Model already extracted:" << targetPath;
        return targetPath;
    }

    // 从 Qt 资源读取
    QFile resFile("assets:/models/model.bin");
    if (!resFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open resource file: /assets/model.bin";
        return QString();
    }

    // 写入目标文件
    QFile targetFile(targetPath);
    if (!targetFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot write to:" << targetPath;
        return QString();
    }

    targetFile.write(resFile.readAll());
    targetFile.close();
    resFile.close();

    qDebug() << "Model extracted to:" << targetPath;
    return targetPath;
}

void MainWindow::onSelectAudioFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    tr("Select Audio File"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                                                    tr("Audio Files (*.wav *.pcm);;All Files (*)"));
    if (!filePath.isEmpty()) {
        appendStatus("选择音频文件: " + filePath);
        
        if (m_whisperManager) {
            m_whisperManager->processAudioFile(filePath);
            appendStatus("已将音频文件送入模型识别");
        } else {
            appendStatus("模型未初始化");
        }
    }
}