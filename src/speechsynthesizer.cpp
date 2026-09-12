#include "speechsynthesizer.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QtConcurrent>
#include <QtGlobal>

#include <cstring>
#include <utility>

#include "sherpa-onnx/c-api/c-api.h"

namespace {

// 模型是中文 VITS，用 lexicon.txt + tokens.txt 做前端，
// 不需要 espeak-ng-data（那只有 Piper 英文 / kokoro 等模型才要）。
constexpr char kModelFileName[]   = "model.onnx";
constexpr char kLexiconFileName[] = "lexicon.txt";
constexpr char kTokensFileName[]  = "tokens.txt";

// 文本正规化用的规则 FST。刻意只用这三个单独的 .fst，
// **不用** 172.3 MB 的 rule.far —— 官方文档对该模型也只传这三个。
// 见 third_party/sherpa-onnx/README.md。
constexpr char kRuleFstNames[] = "phone.fst,date.fst,number.fst";

// 后端线程数：留给 whisper 与界面一些余量（whisper 自己用 4 线程）。
constexpr int kNumThreads = 2;

// PCM 队列的排空节奏：每 20 ms 放出一批。
constexpr int kDrainIntervalMs = 20;

// 每拍最多放出 16 KiB ≈ 1 秒的 8 kHz int16 音频，远快于真实播放速度，
// 因此正常路径下队列几乎总是空的，上限只作为安全阀存在。
constexpr int kDrainChunkBytes = 16 * 1024;

// 待播 PCM 上限。8 kHz int16 单声道 = 16 KB/s，2 MiB ≈ 2 分钟音频。
// 超过它说明播放端停滞了，此时提前结束合成比让内存无限增长更安全。
constexpr int kMaxQueuedPcmBytes = 2 * 1024 * 1024;

// 传给 sherpa 增量回调的上下文。**分配在 worker 的栈上**，只在这一次
// Generate 调用期间存活；代次号让回调能识别「自己是否已被取消」。
struct GenerateCallContext {
    SpeechSynthesizer *self = nullptr;
    quint64 generationId = 0;
};

} // namespace

SpeechSynthesizer::SpeechSynthesizer(QObject *parent)
    : QObject(parent)
    , m_drainTimer(new QTimer(this))
{
    m_drainTimer->setTimerType(Qt::CoarseTimer);
    m_drainTimer->setInterval(kDrainIntervalMs);
    connect(m_drainTimer, &QTimer::timeout, this, &SpeechSynthesizer::drainQueue);
}

SpeechSynthesizer::~SpeechSynthesizer()
{
    stop();
    // worker 可能仍在 Generate 内部使用 m_tts，必须等它退出再销毁句柄。
    m_future.waitForFinished();
    if (m_tts) {
        SherpaOnnxDestroyOfflineTts(m_tts);
        m_tts = nullptr;
    }
}

bool SpeechSynthesizer::init(const QString &modelDir)
{
    if (m_tts)
        return true;

    const QString dir = QDir::cleanPath(modelDir);

    const QString modelFile   = dir + QLatin1Char('/') + QLatin1String(kModelFileName);
    const QString lexiconFile = dir + QLatin1Char('/') + QLatin1String(kLexiconFileName);
    const QString tokensFile  = dir + QLatin1Char('/') + QLatin1String(kTokensFileName);

    for (const QString &path : {modelFile, lexiconFile, tokensFile}) {
        if (!QFileInfo::exists(path)) {
            emit failed(QStringLiteral("TTS 模型文件缺失: %1").arg(path));
            return false;
        }
    }

    // 存成成员：这两个 QByteArray 必须活到 SherpaOnnxCreateOfflineTts() 返回之后。
    m_modelFile   = modelFile.toUtf8();
    m_lexiconFile = lexiconFile.toUtf8();
    m_tokensFile  = tokensFile.toUtf8();

    QStringList ruleFsts;
    for (const QString &name : QString(QLatin1String(kRuleFstNames)).split(QLatin1Char(','))) {
        const QString path = dir + QLatin1Char('/') + name;
        if (!QFileInfo::exists(path)) {
            emit failed(QStringLiteral("TTS 规则文件缺失: %1").arg(path));
            return false;
        }
        ruleFsts << path;
    }
    m_ruleFsts = ruleFsts.join(QLatin1Char(',')).toUtf8();

    SherpaOnnxOfflineTtsConfig config;
    std::memset(&config, 0, sizeof(config));

    config.model.vits.model      = m_modelFile.constData();
    config.model.vits.lexicon    = m_lexiconFile.constData();
    config.model.vits.tokens     = m_tokensFile.constData();
    config.model.vits.data_dir   = nullptr;    // 中文 VITS 不需要 espeak-ng-data
    config.model.vits.noise_scale   = 0.667f;  // VITS 默认值
    config.model.vits.noise_scale_w = 0.8f;
    config.model.vits.length_scale  = 1.0f;    // 1.0 = 正常语速
    config.model.num_threads = kNumThreads;
    config.model.debug = 0;
    config.model.provider = "cpu";

    config.rule_fsts = m_ruleFsts.constData();
    config.rule_fars = nullptr;      // 刻意不用 rule.far
    config.max_num_sentences = 1;    // 我们已经按句切好，每次只喂一句
    config.silence_scale = 0.2f;

    m_tts = SherpaOnnxCreateOfflineTts(&config);
    if (!m_tts) {
        emit failed(QStringLiteral("TTS 模型加载失败: %1").arg(dir));
        return false;
    }

    m_sampleRate = SherpaOnnxOfflineTtsSampleRate(m_tts);
    const int numSpeakers = SherpaOnnxOfflineTtsNumSpeakers(m_tts);
    qDebug() << "TTS 模型加载成功 sampleRate=" << m_sampleRate
             << "speakers=" << numSpeakers;
    return true;
}

bool SpeechSynthesizer::synthesizeSentence(const QString &text)
{
    // 静默是正常结果：未就绪或空白文本都只是「不排队」，不是错误。
    if (!m_tts)
        return false;
    if (text.trimmed().isEmpty())
        return false;

    enqueue(text);
    tryStartWorker();

    // 上一句的 worker 可能还没退出（于是 tryStartWorker 空转），
    // 必须保证排空定时器在跑，否则这句话会被永久搁置。
    if (!m_drainTimer->isActive())
        m_drainTimer->start();
    return true;
}

void SpeechSynthesizer::stop()
{
    // 把「及此代次为止全部取消」抬到当前代次。只增不减，所以下一次
    // tryStartWorker 拿到的新代次天然是未取消的，而被取消的 worker
    // 不会因为标志被重置而复活。
    m_cancelUpTo.store(m_generationId.load());

    {
        QMutexLocker locker(&m_pendingMutex);
        m_pending.clear();
    }
    {
        QMutexLocker locker(&m_pcmMutex);
        m_pcmQueue.clear();
    }

    m_generationDone.store(false);
    m_generateFailed.store(false);
    m_overflowed.store(false);

    if (m_drainTimer)
        m_drainTimer->stop();

    // 刻意不把 m_generating 置 false：worker 可能还在 Generate 内部，
    // 提前放开会让下一句与它并发使用同一个 tts 句柄。它自己退出时会清掉。
    // 也刻意不发 failed：用户主动停止不是错误。
}

bool SpeechSynthesizer::hasPending() const
{
    QMutexLocker locker(&m_pendingMutex);
    return !m_pending.isEmpty();
}

void SpeechSynthesizer::enqueue(const QString &text)
{
    QMutexLocker locker(&m_pendingMutex);
    m_pending.append(text);
}

void SpeechSynthesizer::tryStartWorker()
{
    if (!m_tts || m_generating.load())
        return;

    QString next;
    {
        QMutexLocker locker(&m_pendingMutex);
        if (m_pending.isEmpty())
            return;
        next = m_pending.takeFirst();
    }

    const quint64 generationId = m_generationId.fetch_add(1) + 1;

    {
        QMutexLocker locker(&m_pcmMutex);
        m_pcmQueue.clear();
    }
    m_generationDone.store(false);
    m_generateFailed.store(false);
    m_overflowed.store(false);
    m_generating.store(true);

    if (!m_drainTimer->isActive())
        m_drainTimer->start();

    m_future = QtConcurrent::run([this, sentence = std::move(next), generationId]() {
        generateInWorker(this, sentence, generationId);
    });
}

void SpeechSynthesizer::drainQueue()
{
    QByteArray ready;
    {
        QMutexLocker locker(&m_pcmMutex);
        if (!m_pcmQueue.isEmpty()) {
            // qMin 是单模板参数，两个实参必须是同一类型，这里显式指定 qsizetype。
            const qsizetype take = qMin<qsizetype>(m_pcmQueue.size(), kDrainChunkBytes);
            ready = m_pcmQueue.left(take);
            m_pcmQueue.remove(0, take);
        }
    }

    if (!ready.isEmpty()) {
        emit audioChunk(ready, m_sampleRate);
        return;   // 队列里很可能还有，留到下一拍
    }

    // 队列已空：若本次生成也收尾了，这一句才算真正结束
    if (m_generationDone.load()) {
        m_generationDone.store(false);
        finishCurrentSentence();
    }

    tryStartWorker();

    // 完全空闲才停表。这里必须重新检查待发队列，避免刚好在两次检查之间
    // 又有数据进来而被漏掉。
    if (!m_generating.load() && !hasPending()) {
        bool queueEmpty = false;
        {
            QMutexLocker locker(&m_pcmMutex);
            queueEmpty = m_pcmQueue.isEmpty();
        }
        if (queueEmpty)
            m_drainTimer->stop();
    }
}

void SpeechSynthesizer::finishCurrentSentence()
{
    if (m_overflowed.exchange(false)) {
        stop();
        emit failed(QStringLiteral("播放队列溢出，已停止合成"));
        return;
    }
    if (m_generateFailed.exchange(false)) {
        stop();
        emit failed(QStringLiteral("语音合成失败"));
        return;
    }
    emit sentenceFinished();
}

bool SpeechSynthesizer::appendPcm(const float *samples, int32_t n)
{
    if (!samples || n <= 0)
        return false;

    // sherpa 给的是 [-1, 1] 的单声道 float，这里按 int16 小端打包。
    QByteArray chunk(n * 2, Qt::Uninitialized);
    char *out = chunk.data();
    for (int32_t i = 0; i < n; ++i) {
        const float v = qBound(-1.0f, samples[i], 1.0f);
        const qint16 s = static_cast<qint16>(qRound(v * 32767.0f));
        out[2 * i]     = static_cast<char>(s & 0xFF);
        out[2 * i + 1] = static_cast<char>((s >> 8) & 0xFF);
    }

    QMutexLocker locker(&m_pcmMutex);
    if (m_pcmQueue.size() + chunk.size() > kMaxQueuedPcmBytes) {
        // 播放端停滞。提前结束合成，由 drain 上报为失败。
        m_overflowed.store(true);
        return false;
    }
    m_pcmQueue.append(chunk);
    return true;
}

int32_t SpeechSynthesizer::onGeneratedAudio(const float *samples, int32_t n,
                                            float progress, void *arg)
{
    Q_UNUSED(progress)

    auto *context = static_cast<GenerateCallContext *>(arg);
    SpeechSynthesizer *self = context->self;

    // 已被取消（按代次记账），或代次已经换了——都立刻停止生成。
    if (context->generationId <= self->m_cancelUpTo.load()
        || context->generationId != self->m_generationId.load()) {
        return 0;
    }

    // 返回 0 表示「提前结束」，1 表示「继续」，与 sherpa 约定一致。
    return self->appendPcm(samples, n) ? 1 : 0;
}

void SpeechSynthesizer::generateInWorker(SpeechSynthesizer *self, QString sentence,
                                         quint64 generationId)
{
    // 上下文分配在**本 worker 的栈上**：只在这次 Generate 调用期间有效，
    // 因此不会有悬垂指针，也不需要任何共享所有权。
    GenerateCallContext context;
    context.self = self;
    context.generationId = generationId;

    bool generated = false;
    if (self->m_tts && generationId > self->m_cancelUpTo.load()) {
        SherpaOnnxGenerationConfig cfg;
        std::memset(&cfg, 0, sizeof(cfg));
        cfg.speed = 1.0f;
        cfg.silence_scale = 0.2f;   // 句间停顿，逐句播放时衔接更自然
        cfg.sid = 0;                // 本模型 174 个发音人，固定用第 0 个

        const QByteArray utf8 = sentence.toUtf8();

        // 用非废弃的 GenerateWithConfig，它同样提供增量回调
        //（参数依次为 samples、n、progress、user arg）。
        const SherpaOnnxGeneratedAudio *audio =
            SherpaOnnxOfflineTtsGenerateWithConfig(self->m_tts, utf8.constData(),
                                                   &cfg,
                                                   &SpeechSynthesizer::onGeneratedAudio,
                                                   &context);
        generated = (audio != nullptr);
        if (audio)
            SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    }

    const bool cancelled = generationId <= self->m_cancelUpTo.load();

    // 收尾只在仍属当前代次时生效，避免污染下一句的状态。
    if (self->m_generationId.load() == generationId) {
        self->m_generationDone.store(true);
        // 被取消或溢出都不算「合成失败」，两者各有自己的上报路径。
        self->m_generateFailed.store(!generated && !cancelled
                                     && !self->m_overflowed.load());
    }

    // 最后才放开串行锁：上面的收尾必须先于下一句的开始。
    self->m_generating.store(false);
}
