#include "devicehandler.h"
#include <QDebug>
#include <QTimer>
#include <QBluetoothUuid>

namespace {

// 无响应模式下普通下行（一整条 AI 回复等）的发包间隔。
// 连续快速写入可能被蓝牙栈丢弃，逐包间隔发送更可靠。
constexpr int kOneShotWriteIntervalMs = 15;

// 流式音频发包节拍的下限来源是发送字节率（见 DeviceHandler::m_streamByteRate）：
// 默认 8 kHz 单声道 int16 = 16000 字节/秒，由 MainWindow 通过 setStreamByteRate()
// 按实际回传采样率设置。开发板也发 8 kHz（板上插值到 16 kHz），因为 Qt Android
// 串行化写入，持续吞吐实测只有约 24 KB/s，撑不住 32 KB/s。

// 发送速率相对音频码率的目标倍率。取 1.5 留出余量：
// 蓝牙连接间隔抖动、重传都会吃掉带宽，刚好等于码率意味着队列只会越堆越高。
constexpr double kStreamRateHeadroom = 1.5;

// 流式发包间隔的上下限。下限防止 MTU 很小时定时器空转（Android 蓝牙栈对
// 高频写入会丢包，所以不再往下压）；上限防止 MTU 很大时出现长时间的静默间隔。
constexpr int kStreamMinTickMs = 10;
constexpr int kStreamMaxTickMs = 50;

// 流式队列积压上限对应的时间窗：约 4 秒音频。用时间而非固定字节数表达，
// 这样 8 kHz（16 KB/s，≈64 KiB）与开发板要求的 16 kHz（32 KB/s，≈128 KiB）
// 拥有相同的安全余量，队列容量不随采样率翻倍而被腰斩。
// 它是安全阀而非常规路径——正常时队列只会有几十毫秒的数据（见 streamTickIntervalMs）。
// 到上限说明蓝牙吞吐确实跟不上音频产生速率，此时停止接收并上报，而不是让内存涨下去。
constexpr int kStreamQueueSeconds = 4;

// 背压水位（毫秒音频）。高水位 1500 ms：队列超过它，调用方应暂停产生音频；
// 低水位 500 ms：降到它以下再恢复。两者之间留出滞回区间，避免在阈值附近
// 反复启停；且高水位远低于 kStreamQueueSeconds，背压正常生效时永远触达不到溢出。
constexpr int kStreamHighWaterMs = 1500;
constexpr int kStreamLowWaterMs  = 500;

// 无响应写入允许同时在途（已交给 Qt、未收到完成回调）的流式分片数。
// 太小会让 Android 栈每个连接事件吃不满；太大则 Java 队列里又会悄悄堆积。
// 4 包 ≈ 2 KB（MTU 517 时），约 60 ms 的音频，足够流水线满速而延迟可忽略。
constexpr int kMaxInflightChunks = 4;

// 在途数长时间不下降（完成回调丢了）时强制清零，避免发送永久停住。
// 略大于 Qt 自己的 3 秒 IO 超时。
constexpr int kInflightStallMs = 3500;

// 连续多少次写入错误才判定链路坏了。单次错误（例如 Qt 的 3 秒 IO 超时）常见且
// 可恢复，丢掉那一包继续即可；连续失败才说明目标真的写不进去。
constexpr int kMaxConsecutiveWriteErrors = 5;

} // namespace

DeviceHandler::DeviceHandler(QObject *parent)
    : QObject(parent)
    , m_discoveryAgent(new QBluetoothDeviceDiscoveryAgent(this))
    , m_controller(nullptr)
    , m_writeTimer(new QTimer(this))
{
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &DeviceHandler::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &DeviceHandler::onScanError);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &DeviceHandler::scanFinished);

    m_writeTimer->setInterval(kOneShotWriteIntervalMs);
    connect(m_writeTimer, &QTimer::timeout, this, &DeviceHandler::onWriteTimerTimeout);
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

    // 捕获局部 controller 指针而非 m_controller：断开后 m_controller 会被置空，
    // 而旧控制器在异步删除期间仍可能发出 connected/errorOccurred 信号。
    connect(m_controller, &QLowEnergyController::connected, this,
            [this, controller = m_controller]() {
        emit statusChanged("Connected. Discovering services...");
        emit connected();
        controller->discoverServices();
    });
    connect(m_controller, &QLowEnergyController::disconnected, this, [this]() {
        emit statusChanged("Disconnected.");
        emit disconnected();
    });
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, [this, controller = m_controller](QLowEnergyController::Error) {
                emit statusChanged("Controller error: " + controller->errorString());
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

    // 清空下行写入状态（待发送队列、定时器、写入目标）
    clearWriteQueue();
    m_writeService = nullptr;
    m_writeCharacteristic = QLowEnergyCharacteristic();

    // 删除所有服务对象
    qDeleteAll(m_services);
    m_services.clear();
    m_pendingServiceDetails = 0;

    if (m_controller) {
        QLowEnergyController *controller = m_controller;
        m_controller = nullptr;
        // disconnectFromDevice() 是异步操作，disconnected 信号在设备实际断开后才发出；
        // 立即 delete 会导致信号丢失（UI 无法感知断开），改为收到信号后再延迟删除。
        connect(controller, &QLowEnergyController::disconnected,
                controller, &QObject::deleteLater);
        controller->disconnectFromDevice();
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

// ---------- 下行写入（AI 回复等 → 眼镜） ----------
bool DeviceHandler::setWriteTarget(const QString &serviceUuid, const QString &charUuid)
{
    // 切换目标时清空尚未发送的队列，避免写入旧特征
    clearWriteQueue();

    QLowEnergyService *service = findService(serviceUuid);
    if (!service) {
        m_writeService = nullptr;
        m_writeCharacteristic = QLowEnergyCharacteristic();
        emit writeError("写入目标服务不存在: " + serviceUuid);
        return false;
    }

    const QLowEnergyCharacteristic ch = service->characteristic(QBluetoothUuid(charUuid));
    if (!ch.isValid()) {
        m_writeService = nullptr;
        m_writeCharacteristic = QLowEnergyCharacteristic();
        emit writeError("写入目标特征不存在: " + charUuid);
        return false;
    }
    if (!(ch.properties() & (QLowEnergyCharacteristic::Write
                             | QLowEnergyCharacteristic::WriteNoResponse))) {
        m_writeService = nullptr;
        m_writeCharacteristic = QLowEnergyCharacteristic();
        emit writeError("该特征不可写: " + charUuid);
        return false;
    }

    m_writeService = service;
    m_writeCharacteristic = ch;
    // 优先使用无响应写入（更快）；仅支持有响应写入的特征回退到 WriteWithResponse
    m_writeMode = (ch.properties() & QLowEnergyCharacteristic::WriteNoResponse)
            ? QLowEnergyService::WriteWithoutResponse
            : QLowEnergyService::WriteWithResponse;

    emit statusChanged("写入目标已设置: " + charUuid);
    return true;
}

bool DeviceHandler::servicePresent(const QString &uuid) const
{
    return findService(uuid) != nullptr;
}

bool DeviceHandler::writeControlValue(const QString &serviceUuid,
                                      const QString &charUuid,
                                      const QByteArray &data)
{
    if (data.isEmpty())
        return false;

    QLowEnergyService *service = findService(serviceUuid);
    if (!service) {
        emit writeError("控制指令服务不存在: " + serviceUuid);
        return false;
    }

    const QLowEnergyCharacteristic ch = service->characteristic(QBluetoothUuid(charUuid));
    if (!ch.isValid()) {
        emit writeError("控制指令特征不存在: " + charUuid);
        return false;
    }

    const auto props = ch.properties();
    if (!(props & (QLowEnergyCharacteristic::Write
                   | QLowEnergyCharacteristic::WriteNoResponse))) {
        emit writeError("控制指令特征不可写: " + charUuid);
        return false;
    }

    const QLowEnergyService::WriteMode mode =
        (props & QLowEnergyCharacteristic::WriteNoResponse)
            ? QLowEnergyService::WriteWithoutResponse
            : QLowEnergyService::WriteWithResponse;
    service->writeCharacteristic(ch, data, mode);
    return true;
}

void DeviceHandler::writeData(const QByteArray &data)
{
    if (!m_writeService || !m_writeCharacteristic.isValid()) {
        emit writeError("未设置写入目标，无法发送数据");
        return;
    }
    if (m_oneShotPending) {
        emit writeError("正在发送上一条数据，请稍候");
        return;
    }
    if (data.isEmpty()) {
        emit writeError("发送内容为空");
        return;
    }

    const int chunkSize = writeChunkSize();
    for (int i = 0; i < data.size(); i += chunkSize) {
        WriteChunk chunk;
        chunk.data = data.mid(i, chunkSize);
        chunk.oneShot = true;
        m_writeQueue.append(chunk);
    }
    m_oneShotPending = true;

    qDebug() << "DeviceHandler::writeData" << data.size() << "bytes ->"
             << m_writeQueue.size() << "chunks, chunkSize =" << chunkSize;

    startSendingIfIdle();
}

bool DeviceHandler::enqueueStreamData(const QByteArray &data)
{
    // 未设置写入目标时静默丢弃：流式音频每几十毫秒来一块，逐个报错会把状态栏刷爆，
    // 「没有回传目标」这件事由调用方（MainWindow）在开启回传时一次性告知用户。
    if (!m_writeService || !m_writeCharacteristic.isValid())
        return false;
    if (data.isEmpty())
        return false;

    // 已经溢出的那一批还没排空之前不再接收新数据。若继续收，就会变成
    // 「丢一段、发一段」的断续噪声；不如让调用方彻底关掉回传。
    if (m_streamOverflowed)
        return false;

    // 积压上限按字节率折算成固定时间窗，随采样率（8 kHz / 16 kHz）自动伸缩。
    const qint64 maxQueuedBytes = qint64(m_streamByteRate) * kStreamQueueSeconds;
    if (m_streamQueuedBytes + data.size() > maxQueuedBytes) {
        m_streamOverflowed = true;
        qWarning().nospace() << "[BLE-ABORT] 原因=队列溢出 queued=" << m_streamQueuedBytes
                             << "B incoming=" << data.size() << "B limit=" << maxQueuedBytes
                             << "B need=" << m_streamByteRate << "B/s";
        emit streamAborted(QString("蓝牙吞吐不足，回传队列积压 %1 KiB")
                           .arg(m_streamQueuedBytes / 1024));
        return false;
    }

    const int chunkSize = writeChunkSize();
    for (int i = 0; i < data.size(); i += chunkSize) {
        WriteChunk chunk;
        chunk.data = data.mid(i, chunkSize);
        chunk.oneShot = false;             // 流式数据不产生 writeFinished
        m_writeQueue.append(chunk);
        m_streamQueuedBytes += chunk.data.size();
    }

    m_streamWasActive = true;
    if (m_streamQueuedBytes > streamHighWaterBytes())
        m_streamAboveHighWater = true;

    startSendingIfIdle();
    return true;
}

qint64 DeviceHandler::streamHighWaterBytes() const
{
    return qint64(m_streamByteRate) * kStreamHighWaterMs / 1000;
}

qint64 DeviceHandler::streamLowWaterBytes() const
{
    return qint64(m_streamByteRate) * kStreamLowWaterMs / 1000;
}

void DeviceHandler::updateStreamWatermarks()
{
    if (m_streamAboveHighWater && m_streamQueuedBytes <= streamLowWaterBytes()) {
        m_streamAboveHighWater = false;
        emit streamLowWater();
    }
}

// 每秒打印一次：出队字节率 vs 音频所需字节率，以及 MTU / 写入模式 / 队列水位。
// 判读：
//   out ≈ need            → 蓝牙吞吐够，积压另有原因
//   out 明显 < need       → 蓝牙是瓶颈；再看 chunk：
//       chunk≈20          → MTU 没协商上去
//       chunk≈500 仍不够  → 连接间隔过大（Android 默认 30~50 ms）或走了有响应写
void DeviceHandler::logStreamThroughput(int bytesJustSent)
{
    // 空闲间隙不算吞吐：队列排空后到下一批音频到达之间什么都没发，若把这段
    // 时间计入窗口，out 会被摊薄成一个看起来像「卡顿」的假低值。
    // 相邻两次出队间隔超过 kStatIdleGapMs 就认为中间空闲过，窗口重新开始。
    constexpr qint64 kStatIdleGapMs = 200;
    if (m_statLastSend.isValid() && m_statLastSend.elapsed() > kStatIdleGapMs) {
        m_statTimer.invalidate();
        m_statBytes = 0;
        m_statChunks = 0;
    }
    m_statLastSend.restart();

    if (!m_statTimer.isValid())
        m_statTimer.start();
    m_statBytes += bytesJustSent;
    ++m_statChunks;

    const qint64 ms = m_statTimer.elapsed();
    if (ms < 1000)
        return;
    qDebug().nospace()
        << "[BLE-TX] out=" << (m_statBytes * 1000 / ms) << " B/s"
        << " need=" << m_streamByteRate << " B/s"
        << " chunk=" << writeChunkSize() << "B"
        << " mtu=" << (m_controller ? m_controller->mtu() : -1)
        << " mode=" << (m_writeMode == QLowEnergyService::WriteWithResponse
                        ? "WithResponse" : "NoResponse")
        << " pkts/s=" << (m_statChunks * 1000 / ms)
        << " queued=" << m_streamQueuedBytes << "B";
    m_statBytes = 0;
    m_statChunks = 0;
    m_statTimer.restart();
}

void DeviceHandler::clearStreamQueue()
{
    m_statTimer.invalidate();
    m_statLastSend.invalidate();
    m_statBytes = 0;
    m_statChunks = 0;
    m_streamQueuedBytes = 0;
    m_streamOverflowed = false;
    m_streamAboveHighWater = false;
    m_streamWasActive = false;
    // 只摘掉流式分片，保留 writeData() 排队中的普通下行数据（顺序不变）。
    // m_writeInFlight 不动：已发出的那包仍在等确认，由 onCharacteristicWritten 收尾。
    for (int i = m_writeQueue.size() - 1; i >= 0; --i) {
        if (!m_writeQueue.at(i).oneShot)
            m_writeQueue.removeAt(i);
    }
    if (m_writeQueue.isEmpty())
        m_writeTimer->stop();
}

int DeviceHandler::writeChunkSize() const
{
    // 分包大小取决于协商后的 ATT MTU（MTU - 3 字节 ATT 头），未知时按默认 23 字节 MTU 处理
    if (m_controller) {
        const int mtu = m_controller->mtu();
        if (mtu > 3)
            return mtu - 3;
    }
    return 20;
}

int DeviceHandler::streamTickIntervalMs() const
{
    const double bytesPerTickTarget = m_streamByteRate * kStreamRateHeadroom;
    const int interval = int(writeChunkSize() * 1000.0 / bytesPerTickTarget);
    return qBound(kStreamMinTickMs, interval, kStreamMaxTickMs);
}

void DeviceHandler::setStreamByteRate(int bytesPerSecond)
{
    if (bytesPerSecond > 0)
        m_streamByteRate = bytesPerSecond;
}

void DeviceHandler::startSendingIfIdle()
{
    if (m_writeQueue.isEmpty())
        return;

    if (m_writeMode == QLowEnergyService::WriteWithResponse) {
        if (m_writeInFlight)
            return;                        // 等 characteristicWritten 回来再发下一包
    } else if (m_writeTimer->isActive()) {
        return;                            // 无响应模式：已有节拍在推进，插队会突发
    }

    sendNextWriteChunk();
}

void DeviceHandler::sendNextWriteChunk()
{
    if (m_writeQueue.isEmpty()) {
        m_writeTimer->stop();
        finishOneShotWriteIfDone();
        return;
    }

    // 无响应写入的闭环流控：在途分片已达上限就先不发，等 characteristicWritten
    // 回来（onCharacteristicWritten 会再次调用本函数）。定时器继续按节拍兜底重试。
    if (m_writeMode == QLowEnergyService::WriteWithoutResponse
            && !m_writeQueue.first().oneShot
            && m_inflightChunks >= kMaxInflightChunks) {
        if (m_lastAckTimer.isValid() && m_lastAckTimer.elapsed() > kInflightStallMs) {
            qWarning() << "[BLE-TX] 完成回调停滞超过" << kInflightStallMs
                       << "ms，重置在途计数 inflight=" << m_inflightChunks;
            m_inflightChunks = 0;      // 回调丢了：放行，避免永久停住
        } else {
            if (!m_writeTimer->isActive())
                m_writeTimer->start();
            return;
        }
    }

    const WriteChunk chunk = m_writeQueue.takeFirst();
    if (!chunk.oneShot) {
        m_streamQueuedBytes -= chunk.data.size();
        updateStreamWatermarks();
    }

    if (m_writeMode == QLowEnergyService::WriteWithoutResponse) {
        // 无响应写入的节拍由定时器给出：普通下行沿用固定间隔，流式音频按 MTU
        // 自适应（见 streamTickIntervalMs）。流式分片另有完成回调约束的在途上限
        // （kMaxInflightChunks），定时器只决定「最快多快」，回调决定「实际多快」。
        m_writeTimer->setInterval(chunk.oneShot ? kOneShotWriteIntervalMs
                                                : streamTickIntervalMs());
    } else {
        m_writeInFlight = true;
    }

    m_writeService->writeCharacteristic(m_writeCharacteristic, chunk.data, m_writeMode);
    if (!chunk.oneShot) {
        if (m_writeMode == QLowEnergyService::WriteWithoutResponse) {
            if (m_inflightChunks == 0)
                m_lastAckTimer.restart();      // 从空闲到有在途：以此刻起计停滞
            ++m_inflightChunks;
        }
        logStreamThroughput(chunk.data.size());
    }

    if (m_writeMode != QLowEnergyService::WriteWithoutResponse)
        return;                            // WriteWithResponse 模式：等 characteristicWritten 确认后再发下一包

    // 无响应模式由定时器按节拍继续发送剩余分片（流式分片同时受在途上限约束）；
    // 若队列已空，停止定时器并立即收尾，避免重复触发 writeFinished
    if (!m_writeQueue.isEmpty()) {
        m_writeTimer->start();
    } else {
        m_writeTimer->stop();
        finishOneShotWriteIfDone();
    }
}

void DeviceHandler::finishOneShotWriteIfDone()
{
    // 队列排空意味着积压已经清掉，下一次回传可以重新尝试（上限只是安全阀）
    m_streamOverflowed = false;

    // 流式队列发空：通知朗读收尾方可以安全让开发板 stop 了（尾音已全部写出）
    if (m_streamWasActive) {
        m_streamWasActive = false;
        emit streamDrained();
    }

    if (!m_oneShotPending)
        return;
    m_oneShotPending = false;
    emit writeFinished();
}

void DeviceHandler::clearWriteQueue()
{
    m_writeTimer->stop();
    m_writeQueue.clear();
    m_streamQueuedBytes = 0;
    m_streamOverflowed = false;
    m_streamAboveHighWater = false;
    m_streamWasActive = false;
    m_oneShotPending = false;
    m_writeInFlight = false;
    m_inflightChunks = 0;
    m_consecutiveWriteErrors = 0;
}

void DeviceHandler::onWriteTimerTimeout()
{
    sendNextWriteChunk();
}

void DeviceHandler::onCharacteristicWritten(const QLowEnergyCharacteristic &c,
                                            const QByteArray &value)
{
    Q_UNUSED(value);
    if (!m_writeCharacteristic.isValid() || c.uuid() != m_writeCharacteristic.uuid())
        return;

    m_consecutiveWriteErrors = 0;          // 有一包成功写出，链路是通的

    if (m_writeMode == QLowEnergyService::WriteWithoutResponse) {
        // Qt Android 对无响应写入同样会发完成回调（Android 栈本地处理完这一包），
        // 用它做在途计数的闭环。oneShot 分片不计入在途，所以这里只在计数>0 时递减。
        if (m_inflightChunks > 0)
            --m_inflightChunks;
        m_lastAckTimer.restart();
        // 在途数腾出空位，马上补发，不必干等下一拍定时器
        if (!m_writeQueue.isEmpty() && m_writeQueue.first().oneShot == false)
            sendNextWriteChunk();
        return;
    }

    m_writeInFlight = false;
    // 队列可能已空（普通下行收尾）也可能还有分片，交给 sendNextWriteChunk 判断
    sendNextWriteChunk();
}

void DeviceHandler::onServiceError(QLowEnergyService::ServiceError error)
{
    // 仅在正在向下行目标写入时报错（其他读/写/描述符操作不归本模块管）
    if (m_writeService != qobject_cast<QLowEnergyService*>(sender()))
        return;
    if (m_writeQueue.isEmpty() && m_inflightChunks == 0)
        return;

    const bool wasStreaming = m_streamQueuedBytes > 0 || m_streamOverflowed
                              || m_inflightChunks > 0;

    // 流式音频的单次写入错误很常见且可恢复：Qt 的 3 秒 IO 超时、Android 栈偶发
    // 忙都会上报 CharacteristicWriteError，但后面的包照样能写出去。以前这里一次
    // 就清空队列并中止回传，表现为「跑一阵子就自己掉回本机播放，而且每次时长不定」。
    // 现在只在**连续**多次失败（没有任何一包成功写出）时才判定链路真的坏了。
    // 成功回调会在 onCharacteristicWritten 里把计数清零。
    if (wasStreaming) {
        ++m_consecutiveWriteErrors;
        // 这一包出错也会收到一次完成/错误回调，不会再递减在途数，这里补上
        if (m_inflightChunks > 0)
            --m_inflightChunks;
        qWarning().nospace() << "[BLE-WRITE-ERR] error=" << int(error)
                             << " 连续=" << m_consecutiveWriteErrors << "/"
                             << kMaxConsecutiveWriteErrors
                             << " queued=" << m_streamQueuedBytes << "B inflight="
                             << m_inflightChunks;
        if (m_consecutiveWriteErrors < kMaxConsecutiveWriteErrors)
            return;                        // 容忍：丢这一包，继续发后面的
    }

    qWarning().nospace() << "[BLE-ABORT] 原因=服务写入错误 error=" << int(error)
                         << " 连续=" << m_consecutiveWriteErrors
                         << " queued=" << m_streamQueuedBytes << "B streaming=" << wasStreaming
                         << " mode=" << (m_writeMode == QLowEnergyService::WriteWithResponse
                                         ? "WithResponse" : "NoResponse");

    clearWriteQueue();
    emit writeError("特征写入失败，错误码: " + QString::number(int(error)));
    if (wasStreaming)
        emit streamAborted("写入失败，错误码: " + QString::number(int(error)));
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
    // 取发信号的 controller，并确认它仍是当前控制器。
    // 断开后 m_controller 会被置空、重连时会被新控制器取代，而旧控制器在异步删除
    // 期间仍可能把已排队的结果发出来——connectToDevice 里为 connected/errorOccurred
    // 处理的是同一个竞态。所以这里既不能直接解引用成员（空指针崩溃），也不能接受
    // 旧控制器的事件（会把属于将亡控制器的 QLowEnergyService 塞进 m_services）。
    QLowEnergyController *controller = qobject_cast<QLowEnergyController*>(sender());
    if (!controller) {
        qWarning() << "serviceDiscovered from unexpected sender, ignored:"
                   << newService.toString();
        return;
    }
    if (controller != m_controller) {
        return;   // 来自已被取代或已断开的旧控制器，忽略
    }

    // 为每个发现的服务创建 QLowEnergyService 对象
    QLowEnergyService *service = controller->createServiceObject(newService, this);
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
    connect(service, &QLowEnergyService::characteristicWritten,
            this, &DeviceHandler::onCharacteristicWritten);
    connect(service, &QLowEnergyService::errorOccurred,
            this, &DeviceHandler::onServiceError);

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
    // 只转发当前激活的特征数据。
    // 除特征外还必须比对服务：本信号是按服务分别连接的（见 onServiceDiscovered），
    // 而同一特征 UUID 可以出现在不同服务里，只比 UUID 会让非激活服务的同名特征
    // 数据混入转写缓冲。此判据与 enableCharacteristicNotification 关闭上一条通知时
    // 的比对保持一致（服务 + 特征）。
    QLowEnergyService *service = qobject_cast<QLowEnergyService*>(sender());
    if (m_activeService && service == m_activeService
            && m_activeCharacteristic.uuid() == c.uuid()) {
        emit dataReceived(value);
    }
}

// ---------- 辅助函数 ----------
QLowEnergyService* DeviceHandler::findService(const QString &uuid) const
{
    // 必须按 QBluetoothUuid 比较，不能比字符串：Qt 6 的 QBluetoothUuid::toString()
    // 返回带花括号的 "{4fafc201-...}"，而 BoardProtocol 的常量不带花括号，
    // 字符串比较永远不等，开发板自动识别因此失效。QBluetoothUuid(QString) 两种写法都能解析，
    // 且 QUuid 比较不区分大小写。
    const QBluetoothUuid target{uuid};
    if (target.isNull())
        return nullptr;
    for (QLowEnergyService *s : m_services) {
        if (s->serviceUuid() == target)
            return s;
    }
    return nullptr;
}