#include "devicehandler.h"
#include <QDebug>
#include <QTimer>
#include <QBluetoothUuid>

namespace {

// 无响应模式下普通下行（一整条 AI 回复等）的发包间隔。
// 连续快速写入可能被蓝牙栈丢弃，逐包间隔发送更可靠。
constexpr int kOneShotWriteIntervalMs = 15;

// 流式音频发包节拍的下限来源是发送字节率（见 DeviceHandler::m_streamByteRate）：
// 默认 8 kHz 单声道 int16 = 16000 字节/秒；开发板要求 16 kHz 时由 MainWindow
// 通过 setStreamByteRate() 提到 32000。发送必须比音频产生快，积压才不会被耗尽。

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

    startSendingIfIdle();
    return true;
}

void DeviceHandler::clearStreamQueue()
{
    m_streamQueuedBytes = 0;
    m_streamOverflowed = false;
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

    const WriteChunk chunk = m_writeQueue.takeFirst();
    if (!chunk.oneShot)
        m_streamQueuedBytes -= chunk.data.size();

    if (m_writeMode == QLowEnergyService::WriteWithoutResponse) {
        // 无响应写入没有完成回调，只能靠定时器控制节拍：普通下行沿用固定间隔，
        // 流式音频按 MTU 自适应（见 streamTickIntervalMs）。
        m_writeTimer->setInterval(chunk.oneShot ? kOneShotWriteIntervalMs
                                                : streamTickIntervalMs());
    } else {
        m_writeInFlight = true;
    }

    m_writeService->writeCharacteristic(m_writeCharacteristic, chunk.data, m_writeMode);

    if (m_writeMode != QLowEnergyService::WriteWithoutResponse)
        return;                            // WriteWithResponse 模式：等 characteristicWritten 确认后再发下一包

    // 无响应模式没有完成回调，由定时器按节拍继续发送剩余分片；
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
    m_oneShotPending = false;
    m_writeInFlight = false;
}

void DeviceHandler::onWriteTimerTimeout()
{
    sendNextWriteChunk();
}

void DeviceHandler::onCharacteristicWritten(const QLowEnergyCharacteristic &c,
                                            const QByteArray &value)
{
    Q_UNUSED(value);
    if (m_writeMode != QLowEnergyService::WriteWithResponse)
        return;
    if (!m_writeCharacteristic.isValid() || c.uuid() != m_writeCharacteristic.uuid())
        return;

    m_writeInFlight = false;
    // 队列可能已空（普通下行收尾）也可能还有分片，交给 sendNextWriteChunk 判断
    sendNextWriteChunk();
}

void DeviceHandler::onServiceError(QLowEnergyService::ServiceError error)
{
    // 仅在正在向下行目标写入时报错（其他读/写/描述符操作不归本模块管）
    if (m_writeService != qobject_cast<QLowEnergyService*>(sender()))
        return;
    if (m_writeQueue.isEmpty())
        return;

    // 流式音频报错时还要额外上报：写失败会一直失败，继续按真实时间喂数据
    // 只会反复报错。让调用方关掉回传才是出口。
    const bool wasStreaming = m_streamQueuedBytes > 0 || m_streamOverflowed;

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
    for (QLowEnergyService *s : m_services) {
        if (s->serviceUuid().toString() == uuid)
            return s;
    }
    return nullptr;
}