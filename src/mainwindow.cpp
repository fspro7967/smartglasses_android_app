#include "mainwindow.h"
#include "assetextractor.h"
#include "sentencesplitter.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIODevice>
#include <QMediaDevices>
#include <QPermissions>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtConcurrent>
#include <QtGlobal>

#include <utility>

namespace {

// TTS 模型在 APK assets 里的目录，以及必须随 APK 打包的全部文件。
// 刻意**不含** rule.far（172.3 MB，官方文档对该模型从不引用它），
// 理由见 third_party/sherpa-onnx/README.md。
constexpr char kTtsAssetDir[]    = "assets:/models/tts-vits-icefall-zh-aishell3";
constexpr char kTtsModelDirName[] = "tts-vits-icefall-zh-aishell3";

QStringList ttsModelFileNames()
{
    return {
        QStringLiteral("model.onnx"),
        QStringLiteral("lexicon.txt"),
        QStringLiteral("tokens.txt"),
        QStringLiteral("phone.fst"),
        QStringLiteral("date.fst"),
        QStringLiteral("number.fst"),
        QStringLiteral("new_heteronym.fst"),
        QStringLiteral("speakers.txt"),
    };
}

// AppDataLocation 下 TTS 模型副本的落地目录。
QString ttsModelTargetDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
         + QLatin1Char('/') + QLatin1String(kTtsModelDirName);
}

qint16 readInt16Le(const char *data, int index)
{
    const int offset = index * 2;
    return static_cast<qint16>(static_cast<quint8>(data[offset])
             | (static_cast<quint16>(static_cast<quint8>(data[offset + 1])) << 8));
}

void writeInt16Le(char *data, int index, qint16 value)
{
    const int offset = index * 2;
    data[offset]     = static_cast<char>(value & 0xFF);
    data[offset + 1] = static_cast<char>((value >> 8) & 0xFF);
}

// 设备不支持模型采样率时的兜底：单声道 int16 线性插值重采样。
//
// QAudioSink **不会**替我们转换采样率——它要求传入的 QAudioFormat 被设备支持，
// 否则就是没有声音。Android 的原生输出通常是 48 kHz，而
// QAudioDevice::isFormatSupported() 未必认 8 kHz，所以这层兜底不能省：
// 规格对步骤 6 的判据是「能听到声音」，直接无声等于判据不成立。
// 语音场景下线形插值足够，不需要更高阶的抗混叠滤波器。
QByteArray resampleLinearLe(const QByteArray &pcm, int fromRate, int toRate)
{
    if (fromRate <= 0 || toRate <= 0 || fromRate == toRate || pcm.size() < 4)
        return pcm;

    const int srcCount = static_cast<int>(pcm.size() / 2);
    const qint64 dstCount = static_cast<qint64>(srcCount) * toRate / fromRate;
    if (dstCount <= 0)
        return QByteArray();

    QByteArray out(static_cast<int>(dstCount) * 2, Qt::Uninitialized);
    const char *in = pcm.constData();
    char *outData = out.data();

    for (qint64 i = 0; i < dstCount; ++i) {
        // 用 16.16 定点定位源样本位置，避免浮点误差累积导致尾部漂移。
        const qint64 pos   = i * fromRate * 65536 / toRate;
        const qint64 index = pos >> 16;
        const int frac     = static_cast<int>(pos & 0xFFFF);

        const int i0 = static_cast<int>(qMin<qint64>(index, srcCount - 1));
        const int i1 = static_cast<int>(qMin<qint64>(index + 1, srcCount - 1));

        const qint16 s0 = readInt16Le(in, i0);
        const qint16 s1 = readInt16Le(in, i1);
        const int interpolated =
            s0 + static_cast<int>((static_cast<qint64>(s1 - s0) * frac) >> 16);

        writeInt16Le(outData, static_cast<int>(i), static_cast<qint16>(interpolated));
    }
    return out;
}

// 播放端积压的低水位。合成比播放快好几倍，若不加节制地一路合成下去，
// 音频会全部堆在 m_pendingPcm 里，最后撞上安全阀并谎报「音频输出停滞」——
// 那是把安全阀当流量控制用。所以只在积压降到低水位时才喂下一句：
// 合成的推进由**播放进度**驱动，上限只作为真正的安全阀存在。
// 64 KiB ≈ 4 秒的 8 kHz int16 音频。
constexpr int kBacklogLowWaterBytes = 64 * 1024;

// 消费者（播放）侧的安全阀上限，与合成器内部的 kMaxQueuedPcmBytes 是两个
// 不同层级的缓冲：那个拦的是「还没交给播放端的音频」，这个拦的是「播放端
// 收下了但设备还没消化掉的积压」。两者的关系见 onAudioChunk() 里的说明。
constexpr int kMaxPlayerBacklogBytes = 4 * 1024 * 1024;

// 排空定时器的周期。设备在 8 kHz 下每 20 ms 只消耗 320 字节。
constexpr int kAudioPumpIntervalMs = 20;

// 连续多少次排空一个字节都写不进去就判定设备无响应。
// 150 × 20 ms = 3 秒。设备正常播放时每 20 ms 都会腾出空间，所以连续 3 秒
// 写不进任何数据只可能是它卡住了。
//
// 为什么必须有这个检测：背压闸门（kBacklogLowWaterBytes）会让合成在积压高时
// 暂停，于是「设备卡住 → 积压不涨 → 安全阀永不触发 → 合成与播放双双静止」
// 会变成一个静默的死局，界面永远停在「正在朗读」，比直接报错更难排查。
constexpr int kStalledTicksBeforeGivingUp = 150;

} // namespace

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

    // 初始化 Whisper：模型直接从 assets 读进内存，不再解压到磁盘。
    // 因此 AppDataLocation 下不会再出现 whisper 模型文件。
    m_whisperManager = new WhisperManager(this);
    connect(m_whisperManager, &WhisperManager::transcriptionReady,
            this, &MainWindow::onTranscriptionResult, Qt::QueuedConnection);
    connect(m_whisperManager, &WhisperManager::errorOccurred,
            this, &MainWindow::onWhisperError, Qt::QueuedConnection);

    if (!m_whisperManager->init(QStringLiteral("assets:/models/model.bin"))) {
        emit statusMessage("Whisper 模型加载失败，请检查文件！");
    } else {
        emit statusMessage("Whisper 模型加载成功！");
    }

    // 初始化语音合成。模型提取与 ONNX 解析都很慢，全部放到后台线程，
    // 构造函数里只接线（规格步骤 2 的硬要求：不占用构造函数）。
    m_synthesizer = new SpeechSynthesizer(this);
    connect(m_synthesizer, &SpeechSynthesizer::audioChunk,
            this, &MainWindow::onAudioChunk, Qt::QueuedConnection);
    connect(m_synthesizer, &SpeechSynthesizer::sentenceFinished,
            this, &MainWindow::onSentenceFinished, Qt::QueuedConnection);
    connect(m_synthesizer, &SpeechSynthesizer::failed,
            this, &MainWindow::onSpeechFailed, Qt::QueuedConnection);
    connect(this, &MainWindow::ttsPrepared, this, &MainWindow::onTtsPrepared);
    connect(this, &MainWindow::ttsPrepareFailed, this, &MainWindow::onTtsPrepareFailed);

    // 排空定时器：把 m_pendingPcm 里的音频按设备的消耗速率持续写给 QAudioSink。
    // 它**不能**只在收到 audioChunk 时触发（那是这个 bug 的成因）：合成比播放快
    // 好几倍，合成结束后就不再有新块到达，靠新块驱动会让尾部音频永远写不出去。
    m_audioPump = new QTimer(this);
    m_audioPump->setTimerType(Qt::PreciseTimer);
    m_audioPump->setInterval(kAudioPumpIntervalMs);
    connect(m_audioPump, &QTimer::timeout, this, &MainWindow::writePendingAudio);

    prepareTtsModel();

    // 初始化网络服务（AI 大模型调用）
    connect(m_msgsender, &MsgSender::responseReceived, this, &MainWindow::onAIResponse);
    // 静默路径之三：「应答」失败**不出声**。这里只写界面与日志，
    // 绝不会走到 speakReply()——所以 TTS 不会把错误信息念出来。
    connect(m_msgsender, &MsgSender::errorOccurred, this,
            [this](const QString &err) { emit statusMessage("AI API 错误: " + err); });

    requestAndroidPermissions();
}

MainWindow::~MainWindow()
{
    // 后台的 TTS 提取/初始化任务捕获了 this，并且会调用 m_synthesizer。
    // 必须等它们结束再让 QObject 的子对象析构，否则退出时可能访问已析构对象。
    // （m_synthesizer 自己也有一套等待，它保证不再有 worker 在用着 tts 句柄。）
    for (auto &future : m_ttsFutures)
        future.waitForFinished();
}

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

    // ==================== 静默策略（规格步骤 7） ====================
    // 三条路径逐条挡住，任何一条都**不会**走到 speakReply()：
    //
    //   路径一：回复文本为空            → 在这里 return
    //   路径二："[无识别结果]"          → 已在 onTranscriptionResult 被挡在
    //                                     「应答」之外，这里再兜一次
    //   路径三：「应答」失败            → 根本到不了本函数。MsgSender 的错误
    //                                     只接到 stateMessage（见构造函数），
    //                                     所以失败信息只会显示、不会被念出来
    if (response.trimmed().isEmpty())
        return;
    if (response == QStringLiteral("[无识别结果]"))
        return;

    m_lastReply = response;   // 供「重播」按钮使用
    speakReply(response);
}

void MainWindow::onWhisperError(const QString &error)
{
    emit statusMessage("Whisper 错误: " + error);
}

// ==================== 朗读（TTS） ====================
//
// 说明：extractModelToFile() 已随规格步骤 3 一并删除。whisper 改为从内存加载后
// 它没有任何调用方；留下的将是死代码，而它「文件存在即有效」的判定正是规格点名
// 要修掉的缺陷。TTS 模型改用 AssetExtractor，见下。

void MainWindow::prepareTtsModel()
{
    if (m_ttsPreparing)
        return;
    m_ttsPreparing = true;

    const QString assetDir      = QLatin1String(kTtsAssetDir);
    const QString targetDir     = ttsModelTargetDir();
    const QStringList fileNames = ttsModelFileNames();

    // 分块复制 31 MB + 解析 29 MB ONNX 都很慢，必须全部在后台线程做，
    // 绝不占用构造函数。结果通过信号投递回 GUI 线程（跨线程 emit 自动排队；
    // 下面两个 connect 必须是默认的 AutoConnection，不能改成 DirectConnection）。
    //
    // m_synthesizer->init() 在这里跨线程调用：此时它必然未就绪、也没有任何
    // 合成在跑（未就绪时 synthesizeSentence 直接被拒绝），因此是安全的。
    QFuture<void> future = QtConcurrent::run([this, assetDir, targetDir, fileNames]() {
        QString error;
        if (!AssetExtractor::ensureExtracted(assetDir, targetDir, fileNames, &error)) {
            emit ttsPrepareFailed(error);
            return;
        }
        if (!m_synthesizer) {
            emit ttsPrepareFailed(QStringLiteral("合成器未创建"));
            return;
        }
        if (!m_synthesizer->init(targetDir)) {
            // SpeechSynthesizer::init() 自己也发过 failed()（带具体原因）。
            // 这里再报一次是为了让 m_ttsPreparing 复位、界面状态明确：只在那边
            // 上报的话，本分支就没有出口，状态会永远停在「正在准备」。
            emit ttsPrepareFailed(QStringLiteral("TTS 模型初始化失败，详见日志"));
            return;
        }
        emit ttsPrepared(m_synthesizer->sampleRate());
    });
    m_ttsFutures.append(future);
}

void MainWindow::onTtsPrepared(int sampleRate)
{
    m_ttsPreparing = false;
    setupAudioOutput(sampleRate);
    m_ttsReady = (m_audioSink != nullptr);
    emit ttsReadyChanged();
    emit statusMessage(m_ttsReady
        ? QString("语音合成就绪（%1 Hz）").arg(sampleRate)
        : QStringLiteral("语音合成就绪，但音频输出不可用"));
}

void MainWindow::onTtsPrepareFailed(const QString &error)
{
    m_ttsPreparing = false;
    m_ttsReady = false;
    emit ttsReadyChanged();
    emit statusMessage("语音合成准备失败: " + error);
}

void MainWindow::setupAudioOutput(int modelSampleRate)
{
    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        emit statusMessage("找不到音频输出设备，无法朗读");
        return;
    }

    // 规格步骤 6：QAudioFormat 用模型真实采样率（8 kHz）与 int16。
    QAudioFormat format;
    format.setSampleRate(modelSampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    m_needResample = false;
    if (!device.isFormatSupported(format)) {
        // QAudioSink 不做采样率转换：格式不被支持就是没声音。
        // 退回设备自己的采样率，由 resampleLinearLe() 补上转换。
        QAudioFormat fallback = device.preferredFormat();
        fallback.setChannelCount(1);
        fallback.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(fallback)) {
            emit statusMessage("音频输出设备不支持单声道 int16 格式，无法朗读");
            return;
        }
        qDebug() << "音频设备不支持" << modelSampleRate << "Hz，改用"
                 << fallback.sampleRate() << "Hz 并自行重采样";
        format = fallback;
        m_needResample = true;
    }

    delete m_audioSink;          // 允许重复初始化
    m_audioSink = new QAudioSink(device, format, this);
    m_audioSampleRate = format.sampleRate();

    // 200 ms 的缓冲：够稳，同时让「停止」足够跟手（另外停止时还会 reset 丢弃缓冲）。
    m_audioSink->setBufferSize(m_audioSampleRate / 5 * static_cast<int>(sizeof(qint16)));

    m_audioDevice = m_audioSink->start();
    if (!m_audioDevice) {
        emit statusMessage("音频输出打开失败");
        delete m_audioSink;
        m_audioSink = nullptr;
        return;
    }
    qDebug() << "音频输出已打开" << m_audioSampleRate << "Hz 单声道 int16"
             << "重采样:" << m_needResample;
}

void MainWindow::onAudioChunk(const QByteArray &pcm, int sampleRate)
{
    if (pcm.isEmpty())
        return;

    // 模型采样率与设备采样率不一致时，在这里补一次重采样。
    m_pendingPcm.append(m_needResample
        ? resampleLinearLe(pcm, sampleRate, m_audioSampleRate)
        : pcm);

    // 消费者侧的安全阀。合成器自己有 2 MiB 的 kMaxQueuedPcmBytes，但**拦不住
    // 这种情况**：数据一旦从那边 drain 出来就归这里了，若音频设备停滞，这个
    // 缓冲会一直涨。到上限就整体停止，而不是让内存无限增长。
    if (m_pendingPcm.size() > kMaxPlayerBacklogBytes) {
        emit statusMessage("音频输出停滞，已停止朗读");
        stopSpeaking();
        return;
    }

    writePendingAudio();

    // 启动排空定时器。它独立于「音频块到达」持续运行，直到积压被写空
    // （见 writePendingAudio 末尾）。这一步是这个 bug 的修复核心。
    if (!m_pendingPcm.isEmpty() && !m_audioPump->isActive())
        m_audioPump->start();
}

void MainWindow::writePendingAudio()
{
    if (!m_audioSink || !m_audioDevice) {
        if (m_audioPump)
            m_audioPump->stop();
        return;
    }

    // 被系统打断而挂起时先恢复，否则写进去也不会出声。
    //
    // 这里刻意**不**比较 state() == SuspendedState：那个枚举所在的命名空间
    // 在 Qt 6.6 之后由 QAudio 改名为 QtAudio，写死任一个都会绑死 Qt 版本。
    // QAudioSink::resume() 的文档明确写着「非挂起状态下本函数什么都不做」，
    // 所以直接无条件调用即可，行为等价且不受这次改名影响。
    m_audioSink->resume();

    qint64 writtenTotal = 0;
    while (!m_pendingPcm.isEmpty()) {
        const qsizetype free = m_audioSink->bytesFree();
        if (free <= 0)
            break;                       // 设备缓冲满了，剩下的留到下次
        const qsizetype take = qMin(free, m_pendingPcm.size());
        const qint64 written = m_audioDevice->write(m_pendingPcm.constData(), take);
        if (written <= 0)
            break;
        writtenTotal += written;
        m_pendingPcm.remove(0, static_cast<qsizetype>(written));
    }

    // 设备无响应检测：有积压却连续几秒一个字节都写不进去。见
    // kStalledTicksBeforeGivingUp 的说明——这是背压闸门引入的失败模式。
    if (m_speaking && !m_pendingPcm.isEmpty() && writtenTotal == 0) {
        if (++m_stalledTicks >= kStalledTicksBeforeGivingUp) {
            m_stalledTicks = 0;
            emit statusMessage("音频输出无响应，已停止朗读");
            stopSpeaking();
            return;
        }
    } else {
        m_stalledTicks = 0;
    }

    // 写空了就停表，避免空转。
    if (m_pendingPcm.isEmpty() && m_audioPump->isActive())
        m_audioPump->stop();

    // 由**播放进度**驱动合成：积压降到低水位才喂下一句，因此合成器与这里的
    // 两道上限都只是安全阀，不会在正常长回复里被触发（见 kBacklogLowWaterBytes）。
    if (m_speakIndex < m_speakQueue.size()
        && m_pendingPcm.size() <= kBacklogLowWaterBytes) {
        speakNextSentence();
    }

    // 最后一块音频已经交给设备、且所有句子都已喂完并合成完，才算朗读结束。
    maybeFinishSpeaking();
}

void MainWindow::maybeFinishSpeaking()
{
    if (!m_speaking)
        return;
    // 三个条件缺一不可。只看 m_pendingPcm 会误判：单句回复时第一个音频块
    // 一写出缓冲就空了，但合成其实才刚开始、设备也还没播。
    if (m_speakIndex < m_speakQueue.size())
        return;                        // 还有句子没喂给合成器
    if (m_sentencesOutstanding > 0)
        return;                        // 还有句子在合成中
    if (!m_pendingPcm.isEmpty())
        return;                        // 还有音频没交给设备

    setSpeaking(false);
}

void MainWindow::setSpeaking(bool speaking)
{
    if (m_speaking == speaking)
        return;
    m_speaking = speaking;
    emit speakingChanged();
}

void MainWindow::stopPlayback()
{
    if (m_audioSink) {
        // reset() 丢弃已缓冲的音频，所以「停止」立即生效，
        // 不用等设备把缓冲区播完。
        m_audioSink->reset();
        m_audioDevice = m_audioSink->start();
    }
    m_pendingPcm.clear();
    if (m_audioPump)
        m_audioPump->stop();
}

void MainWindow::speakReply(const QString &reply)
{
    if (reply.trimmed().isEmpty())
        return;

    if (!m_ttsReady || !m_synthesizer || !m_synthesizer->isReady()) {
        emit statusMessage("语音合成未就绪，本次回复不朗读");
        return;
    }

    // 规格步骤 5/6：逐句合成。切句是纯函数，已由 tests/ 单独验证。
    m_speakQueue = splitSentences(reply);
    m_speakIndex = 0;
    m_sentencesOutstanding = 0;
    if (m_speakQueue.isEmpty())
        return;

    setSpeaking(true);
    speakNextSentence();
}

void MainWindow::speakNextSentence()
{
    if (!m_synthesizer || !m_synthesizer->isReady()) {
        setSpeaking(false);
        return;
    }

    // 背压：积压还高于低水位就先不喂。writePendingAudio() 会在积压降下来后
    // 再调一次本函数，所以这里返回不会让流水线停住；反过来，不加这道闸门
    // 会让合成（比播放快好几倍）把音频全堆进 m_pendingPcm。
    if (m_pendingPcm.size() > kBacklogLowWaterBytes)
        return;

    // 一句合成完（sentenceFinished）再喂下一句，这样任何时刻堆积的音频
    // 都不超过一两句，合成器与这里的两道上限都只是安全阀而非常规路径。
    while (m_speakIndex < m_speakQueue.size()) {
        const QString sentence = m_speakQueue.at(m_speakIndex++);
        if (m_synthesizer->synthesizeSentence(sentence)) {
            ++m_sentencesOutstanding;
            return;
        }
        // 对空白文本 synthesizeSentence 返回 false（静默是正常结果）。
        // 必须跳过它继续找下一句，否则整条流水线会永久卡住。
    }

    // 全部句子都已喂出，等最后一句的 sentenceFinished 收尾。
    maybeFinishSpeaking();
}

void MainWindow::onSentenceFinished()
{
    if (m_sentencesOutstanding > 0)
        --m_sentencesOutstanding;
    speakNextSentence();
}

void MainWindow::onSpeechFailed(const QString &reason)
{
    m_sentencesOutstanding = 0;
    setSpeaking(false);
    emit statusMessage("语音合成错误: " + reason);
}

void MainWindow::stopSpeaking()
{
    if (m_synthesizer)
        m_synthesizer->stop();       // 用户主动停止，合成器不会发 failed
    m_speakQueue.clear();
    m_speakIndex = 0;
    m_sentencesOutstanding = 0;
    stopPlayback();
    setSpeaking(false);
}

void MainWindow::replayLastReply()
{
    if (m_lastReply.trimmed().isEmpty()) {
        emit statusMessage("没有可重播的回复");
        return;
    }
    stopSpeaking();
    speakReply(m_lastReply);
}
