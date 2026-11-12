#include "whisper_manager.h"
#include "wavpcm.h"
#include "whisper.h"

#include <QDebug>
#include <QFile>
#include <QtConcurrent>

#include <sys/resource.h>
#include <utility>

namespace {

float toFloat(qint16 sample)
{
    return static_cast<float>(sample) / 32768.0f;
}

qint16 le16(char lo, char hi)
{
    return static_cast<qint16>(static_cast<quint16>(static_cast<quint8>(lo))
                               | (static_cast<quint16>(static_cast<quint8>(hi)) << 8));
}

} // namespace

WhisperManager::WhisperManager(QObject *parent)
    : QObject(parent)
{
    m_window.reserve(kWindowSamples);

    // worker 线程收工后在对象线程里推进下一段。必须排队：m_busy / m_pending
    // 只允许对象线程碰，这条连接就是它们的同步手段。
    connect(this, &WhisperManager::segmentFinished, this, [this]() {
        m_busy = false;
        startNextIfIdle();
    }, Qt::QueuedConnection);
}

WhisperManager::~WhisperManager()
{
    // 后台任务都捕获了 this，并且在用 m_ctx：必须等它们退出再释放。
    m_initFuture.waitForFinished();
    m_workFuture.waitForFinished();
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

void WhisperManager::initAsync(const QString &modelAssetPath)
{
    if (m_ready.load() || m_initFuture.isRunning())
        return;

    m_initFuture = QtConcurrent::run([this, modelAssetPath]() {
        QString error;
        if (init(modelAssetPath, &error)) {
            m_ready.store(true);     // m_ctx 已写好，此后只读
            emit ready();
        } else {
            emit initFailed(error);
        }
    });
}

bool WhisperManager::init(const QString &modelAssetPath, QString *errorOut)
{
    QFile modelFile(modelAssetPath);
    if (!modelFile.open(QIODevice::ReadOnly)) {
        *errorOut = "无法读取模型文件: " + modelAssetPath;
        return false;
    }

    // 整块读入内存后直接初始化，不再写盘。whisper 在下面的调用期间同步读完
    // 所有权重，所以这个 QByteArray 出了作用域就能释放掉——不再有一份 59.7 MB
    // 的模型常驻在 AppDataLocation 里。
    QByteArray modelData = modelFile.readAll();
    modelFile.close();

    if (modelData.isEmpty()) {
        *errorOut = "模型文件为空: " + modelAssetPath;
        return false;
    }

    whisper_context_params params = whisper_context_default_params();
    m_ctx = whisper_init_from_buffer_with_params(modelData.data(),
                                                 static_cast<size_t>(modelData.size()),
                                                 params);
    if (!m_ctx) {
        *errorOut = "加载模型失败: " + modelAssetPath;
        return false;
    }
    qDebug() << "Whisper 模型从内存加载成功，字节数" << modelData.size();
    return true;
}

void WhisperManager::appendSample(qint16 sample)
{
    m_window.push_back(toFloat(sample));
    if (m_window.size() >= static_cast<size_t>(kWindowSamples)) {
        enqueueSegment(std::move(m_window));
        m_window.clear();
        m_window.reserve(kWindowSamples);
    }
}

void WhisperManager::feedAudioData(const QByteArray &pcm)
{
    if (pcm.isEmpty() || !m_ready.load())
        return;

    const char *data = pcm.constData();
    const qsizetype size = pcm.size();
    qsizetype i = 0;

    // 上一次留下的半个样本与这一次的第一个字节拼成一个完整样本。
    // 不这样做的话，一个奇数长度的分片之后整条流都会错开一个字节，全是噪声。
    if (m_haveCarry) {
        appendSample(le16(m_carryByte, data[0]));
        m_haveCarry = false;
        i = 1;
    }
    for (; i + 1 < size; i += 2)
        appendSample(le16(data[i], data[i + 1]));
    if (i < size) {
        m_carryByte = data[i];
        m_haveCarry = true;
    }
}

void WhisperManager::flush()
{
    m_haveCarry = false;   // 流已经停了，半个样本没有下文

    if (m_window.size() < static_cast<size_t>(kMinFlushSamples)) {
        m_window.clear();
        return;
    }
    enqueueSegment(std::move(m_window));
    m_window.clear();
    m_window.reserve(kWindowSamples);
}

void WhisperManager::enqueueSegment(std::vector<float> &&segment)
{
    if (segment.empty())
        return;

    // 队列满：丢最早的一段，保住最新说的话，并且必须说出来——
    // 以前忙的时候是整段静默蒸发，表现为「中间一句没识别」而没有任何线索。
    if (m_pending.size() >= static_cast<size_t>(kMaxPendingSegments)) {
        const double droppedSeconds =
            static_cast<double>(m_pending.front().size()) / kSampleRate;
        m_pending.pop_front();
        emit errorOccurred(QString("识别速度跟不上，已丢弃最早的 %1 秒音频")
                           .arg(droppedSeconds, 0, 'f', 1));
    }

    const int sampleCount = static_cast<int>(segment.size());
    m_pending.push_back(std::move(segment));
    emit segmentQueued(sampleCount, static_cast<int>(m_pending.size()));

    startNextIfIdle();
}

void WhisperManager::startNextIfIdle()
{
    if (m_busy || m_pending.empty() || !m_ready.load())
        return;

    m_busy = true;
    std::vector<float> segment = std::move(m_pending.front());
    m_pending.pop_front();
    const quint32 epoch = m_epoch.load();

    qDebug() << "Start Whisper processing with" << segment.size() << "samples,"
             << m_pending.size() << "segments still pending";

    m_workFuture = QtConcurrent::run([this, segment = std::move(segment), epoch]() {
        processBuffer(segment, epoch);
        emit segmentFinished();
    });
}

void WhisperManager::processBuffer(const std::vector<float> &audioChunk, quint32 epoch)
{
    // reset() 之后才轮到的段（或转写途中被 reset 的段）属于上一段会话，结果作废。
    const auto stale = [this, epoch]() { return epoch != m_epoch.load(); };

    if (!m_ctx || audioChunk.empty() || stale())
        return;

    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = 4;
    //params.language = "auto";
    params.print_realtime = false;
    params.print_progress = false;
    params.print_timestamps = false;
    params.print_special = false;
    params.translate = false;
    // **刻意写死 "en"，不要"顺手修"成 "zh" 或 "auto"。**
    //
    // 这是一处有意为之的语言不对称：使用者说英文，而 whisper 的英文识别率明显
    // 高于中文；回复之所以是中文，不由这里决定，而是由 msgsender.cpp 的中文
    // system prompt 决定（kSystemPrompt，API 与 Ollama 两条路共用，
    // 「你是智能眼镜上的语音助手，回答请简洁、口语化。」）。
    //
    // 即：输入英文 → whisper 转写成英文 → 大模型按中文 system prompt 回中文 →
    // 朗读中文。改动这一行会让转写质量下降，而原因不会在报错里出现。
    // 决策记录见 docs/adr/0002-english-in-chinese-out.md。
    params.language = "en";
    params.offset_ms = 0;
    params.no_context = true ;
    params.single_segment = false;
    //排除空白token
    //params.suppress_blank = true;
    //params.suppress_nst = true;
    params.audio_ctx = 278 ;
    //解码与置信度阈值，防止幻觉
    //params.temperature = 0.0f;
    //params.temperature_inc = 0.2f;
    //params.entropy_thold = 2.4f;
    //params.logprob_thold = -1.0f;
    setpriority(PRIO_PROCESS, 0, -10);

    qDebug() << "Calling whisper_full with" << audioChunk.size() << "samples";
    if (whisper_full(m_ctx, params, audioChunk.data(),
                     static_cast<int>(audioChunk.size())) != 0) {
        if (!stale())
            emit errorOccurred("Whisper 转录失败");
        return;
    }
    qDebug() << "Whisper transcription completed successfully";
    whisper_print_timings(m_ctx);

    QString fullText;
    const int n_segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(m_ctx, i);
        if (text)
            fullText += QString::fromUtf8(text);
    }
    qDebug() << "Whisper result text=" << fullText << "segments=" << n_segments;

    if (stale())
        return;

    if (!fullText.isEmpty()) {
        emit transcriptionReady(fullText);
    } else {
        emit errorOccurred("Whisper 未识别到有效文本");
        emit transcriptionReady("[无识别结果]");
    }
}

bool WhisperManager::processAudioFile(const QString &filePath)
{
    if (!m_ready.load()) {
        emit errorOccurred("语音识别模型尚未就绪");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("无法打开音频文件: " + filePath);
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        emit errorOccurred("音频文件为空");
        return false;
    }

    // WAV 要先剥掉文件头并核对格式；裸 PCM 原样使用（见 wavpcm.h）。
    QByteArray pcm;
    QString error;
    if (!WavPcm::extract(data, &pcm, &error)) {
        emit errorOccurred(error);
        return false;
    }

    const size_t sampleCount = static_cast<size_t>(pcm.size() / 2);
    if (sampleCount == 0) {
        emit errorOccurred("没有有效音频数据");
        return false;
    }

    // 文件是用户主动选的，队列满时拒绝它，而不是为它挤掉排队中的上行音频。
    if (m_pending.size() >= static_cast<size_t>(kMaxPendingSegments)) {
        emit errorOccurred("识别队列已满，请稍后再试");
        return false;
    }

    std::vector<float> samples;
    samples.reserve(sampleCount);
    const char *bytes = pcm.constData();
    for (size_t i = 0; i < sampleCount; ++i)
        samples.push_back(toFloat(le16(bytes[2 * i], bytes[2 * i + 1])));

    qDebug() << "WhisperManager::processAudioFile sampleCount=" << sampleCount;
    enqueueSegment(std::move(samples));
    return true;
}

void WhisperManager::reset()
{
    m_window.clear();
    m_haveCarry = false;
    m_pending.clear();
    // 正在 worker 里跑的那一段停不下来，但它的结果从此作废。
    // m_busy 不动：等它的 segmentFinished 回来再放行下一段。
    m_epoch.fetch_add(1);
}
