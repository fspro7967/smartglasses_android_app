#include "mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QPermissions>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QVariantMap>
#include <QVariantList>

MainWindow::MainWindow(QObject *parent)
    : QObject(parent)
    , m_deviceHandler(new DeviceHandler(this))
    , m_msgsender(new MsgSender(this))
{
    // 蓝牙信号转发至 QML
    connect(m_deviceHandler, &DeviceHandler::deviceDiscovered, this,
            [this](const QBluetoothDeviceInfo &info) {
                m_discoveredDevices.append(info);
                const QString name = info.name().isEmpty() ? info.address().toString() : info.name();
                emit deviceDiscovered(name);
            });
    connect(m_deviceHandler, &DeviceHandler::statusChanged, this, &MainWindow::statusMessage);
    connect(m_deviceHandler, &DeviceHandler::dataReceived, this, &MainWindow::onDataReceived);

    connect(m_deviceHandler, &DeviceHandler::connected, this, [this]() {
        m_isConnected = true;
        m_deviceName = m_pendingDeviceName;
        emit connectionChanged();
        clearServices();
        emit statusMessage("已连接设备: " + m_deviceName);
    });
    connect(m_deviceHandler, &DeviceHandler::disconnected, this, [this]() {
        m_isConnected = false;
        m_deviceName.clear();
        emit connectionChanged();
        clearServices();
        emit statusMessage("连接已断开");
    });
    connect(m_deviceHandler, &DeviceHandler::scanFinished, this, [this]() {
        m_isScanning = false;
        emit scanningChanged();
        emit statusMessage("扫描完成");
    });
    connect(m_deviceHandler, &DeviceHandler::serviceDiscovered, this,
            &MainWindow::onServiceDiscovered);
    connect(m_deviceHandler, &DeviceHandler::characteristicDiscovered, this,
            &MainWindow::onCharacteristicDiscovered);

    // 初始化 Whisper
    m_modelPath = extractModelToFile();
    if (m_modelPath.isEmpty()) {
        emit statusMessage("模型文件提取失败！");
    }

    m_whisperManager = new WhisperManager(this);
    connect(m_whisperManager, &WhisperManager::transcriptionReady,
            this, &MainWindow::onTranscriptionResult, Qt::QueuedConnection);
    connect(m_whisperManager, &WhisperManager::errorOccurred,
            this, &MainWindow::onWhisperError, Qt::QueuedConnection);

    if (!m_whisperManager->init(m_modelPath)) {
        emit statusMessage("Whisper 模型加载失败，请检查文件！");
    } else {
        emit statusMessage("Whisper 模型加载成功！");
    }

    // 初始化网络服务（AI 大模型调用）
    connect(m_msgsender, &MsgSender::responseReceived, this, &MainWindow::onAIResponse);
    connect(m_msgsender, &MsgSender::errorOccurred, this,
            [this](const QString &err) { emit statusMessage("AI API 错误: " + err); });

    requestAndroidPermissions();
}

MainWindow::~MainWindow() {}

void MainWindow::startScan()
{
    m_discoveredDevices.clear();
    m_isScanning = true;
    emit scanningChanged();
    emit statusMessage("开始扫描蓝牙设备...");
    m_deviceHandler->startScan();
}

void MainWindow::connectToDevice(int index)
{
    if (index >= 0 && index < m_discoveredDevices.size()) {
        const QBluetoothDeviceInfo &info = m_discoveredDevices.at(index);
        m_pendingDeviceName = info.name().isEmpty() ? info.address().toString() : info.name();
        m_deviceHandler->connectToDevice(info);
    }
}

void MainWindow::disconnectDevice()
{
    m_deviceHandler->disconnectDevice();
    // 主动断开时立即同步 UI 状态（BLE 断开事件异步到达，避免界面滞后）
    m_isConnected = false;
    m_deviceName.clear();
    emit connectionChanged();
    clearServices();
    emit statusMessage("正在断开连接...");
}

void MainWindow::enableNotification(const QString &serviceUuid, const QString &charUuid)
{
    m_deviceHandler->enableCharacteristicNotification(serviceUuid, charUuid, true);
    emit statusMessage("已启用通知: " + charUuid);
}

void MainWindow::processAudioFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    emit statusMessage("选择音频文件: " + filePath);
    if (m_whisperManager) {
        m_whisperManager->processAudioFile(filePath);
        emit statusMessage("已将音频文件送入模型识别");
    } else {
        emit statusMessage("模型未初始化");
    }
}

void MainWindow::sendMessageToServer(const QString &message)
{
    if (m_msgsender) {
        m_msgsender->sendMessage(message);
        emit statusMessage("已发送至 AI 大模型: " + message);
    } else {
        emit statusMessage("消息发送器未初始化");
    }
}

// ==================== AI 大模型接入方式 ====================
int MainWindow::aiProvider() const
{
    return m_msgsender ? static_cast<int>(m_msgsender->provider()) : 0;
}

QString MainWindow::aiProviderName() const
{
    if (!m_msgsender) return QString();
    return m_msgsender->provider() == MsgSender::ProviderOllama
            ? QStringLiteral("本地 Ollama")
            : QStringLiteral("OpenAI 兼容 API");
}

QString MainWindow::apiBaseUrl() const
{
    return m_msgsender ? m_msgsender->apiBaseUrl() : QString();
}

QString MainWindow::apiKey() const
{
    return m_msgsender ? m_msgsender->apiKey() : QString();
}

QString MainWindow::apiModelName() const
{
    return m_msgsender ? m_msgsender->apiModelName() : QString();
}

QString MainWindow::ollamaUrl() const
{
    return m_msgsender ? m_msgsender->ollamaUrl() : QString();
}

QString MainWindow::ollamaModelName() const
{
    return m_msgsender ? m_msgsender->ollamaModelName() : QString();
}

void MainWindow::setAiProvider(int provider)
{
    if (!m_msgsender) return;
    m_msgsender->setProvider(static_cast<MsgSender::Provider>(provider));
    emit aiConfigChanged();
    emit statusMessage(provider == MsgSender::ProviderOllama
                       ? "已切换到本地 Ollama 接入"
                       : "已切换到 OpenAI 兼容 API 接入");
}

void MainWindow::setApiConfig(const QString &baseUrl, const QString &apiKey, const QString &modelName)
{
    if (!m_msgsender) return;
    m_msgsender->setApiConfig(baseUrl, apiKey, modelName);
    emit aiConfigChanged();
    emit statusMessage("API 配置已保存（模型: " + modelName + "）");
}

void MainWindow::setOllamaConfig(const QString &serverUrl, const QString &modelName)
{
    if (!m_msgsender) return;
    m_msgsender->setOllamaConfig(serverUrl, modelName);
    emit aiConfigChanged();
    emit statusMessage("Ollama 配置已保存（模型: " + modelName + "）");
}

void MainWindow::clearServices()
{
    m_services.clear();
    emit servicesChanged();
}

void MainWindow::onServiceDiscovered(const QString &serviceUuid)
{
    QVariantMap service;
    service.insert("type", "service");
    service.insert("serviceUuid", serviceUuid);
    service.insert("label", serviceUuid);
    service.insert("chars", QVariantList());
    m_services.append(service);
    emit servicesChanged();
}

void MainWindow::onCharacteristicDiscovered(const QString &serviceUuid,
                                            const QString &charUuid,
                                            const QString &charName,
                                            int properties)
{
    for (int i = 0; i < m_services.size(); ++i) {
        QVariantMap service = m_services.at(i).toMap();
        if (service.value("type") == "service"
                && service.value("serviceUuid").toString() == serviceUuid) {
            QStringList propsList;
            if (properties & QLowEnergyCharacteristic::Read) propsList << tr("读");
            if (properties & QLowEnergyCharacteristic::Write) propsList << tr("写");
            if (properties & QLowEnergyCharacteristic::Notify) propsList << tr("通知");
            if (properties & QLowEnergyCharacteristic::Indicate) propsList << tr("指示");

            QVariantList chars = service.value("chars").toList();
            QVariantMap ch;
            ch.insert("serviceUuid", serviceUuid);
            ch.insert("charUuid", charUuid);
            ch.insert("charName", charName);
            ch.insert("props", propsList.join(" · "));
            ch.insert("notifiable", (properties & QLowEnergyCharacteristic::Notify)
                                     || (properties & QLowEnergyCharacteristic::Indicate));
            chars.append(ch);
            service.insert("chars", chars);
            m_services.replace(i, service);
            emit servicesChanged();
            break;
        }
    }
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    // 把收到的数据存入缓存区
    m_audioBuffer.append(data);

    // 把缓存的数据传入模型，并清空缓存
    constexpr int CHUNK_SIZE_BYTES = 3 * 16000 * sizeof(int16_t); // 约 3 秒的 16kHz/16bit 音频
    if (m_audioBuffer.size() >= CHUNK_SIZE_BYTES && m_whisperManager) {
        const int16_t *pcmData = reinterpret_cast<const int16_t*>(m_audioBuffer.constData());
        const size_t sampleCount = static_cast<size_t>(m_audioBuffer.size() / sizeof(int16_t));
        emit statusMessage(QString("已将 %1 字节音频送入 Whisper，样本数 %2")
                           .arg(m_audioBuffer.size()).arg(sampleCount));
        m_whisperManager->feedAudioData(pcmData, sampleCount);
        m_audioBuffer.clear();
    }
}

void MainWindow::onBluetoothPermissionGranted()
{
    qDebug() << "Bluetooth permission granted.";
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Approximate);
    qGuiApp->requestPermission(locationPermission, this, [this](const QPermission &perm) {
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
    QGuiApplication *app = qGuiApp;
    if (!app) return;
    app->requestPermission(bluetoothPermission, this, &MainWindow::onBluetoothPermissionGranted);
}

// Whisper 结果回调
void MainWindow::onTranscriptionResult(const QString &text)
{
    emit transcriptionReady(text);
    emit statusMessage("Whisper 识别完成");

    // 将识别结果发送至 AI 大模型（过滤空结果占位符）
    if (!text.isEmpty() && text != "[无识别结果]" && m_msgsender) {
        m_msgsender->sendMessage(text);
        emit statusMessage("已将识别文本发送至 AI 大模型");
    }
}

// AI 大模型回复回调
void MainWindow::onAIResponse(const QString &response)
{
    emit aiResponseReady(response);
}

void MainWindow::onWhisperError(const QString &error)
{
    emit statusMessage("Whisper 错误: " + error);
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
