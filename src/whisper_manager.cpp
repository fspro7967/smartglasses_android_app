#include "whisper_manager.h"
#include "whisper.h"
#include <QDebug>
#include <QtConcurrent>
#include <algorithm>
#include <QFile>

WhisperManager::WhisperManager(QObject *parent)
    : QObject(parent)
{
}

WhisperManager::~WhisperManager()
{
    for (auto &future : m_futures) {
        future.waitForFinished();
    }
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

bool WhisperManager::init(const QString &modelAssetPath)
{
    QFile modelFile(modelAssetPath);
    if (!modelFile.open(QIODevice::ReadOnly)) {
        emit errorOccurred("无法读取模型文件: " + modelAssetPath);
        return false;
    }

    // 整块读入内存后直接初始化，不再写盘。whisper 在下面的调用期间同步读完
    // 所有权重，所以这个 QByteArray 出了作用域就能释放掉——不再有一份 59.7 MB
    // 的模型常驻在 AppDataLocation 里。
    QByteArray modelData = modelFile.readAll();
    modelFile.close();

    if (modelData.isEmpty()) {
        emit errorOccurred("模型文件为空: " + modelAssetPath);
        return false;
    }

    whisper_context_params params = whisper_context_default_params();
    m_ctx = whisper_init_from_buffer_with_params(modelData.data(),
                                                 static_cast<size_t>(modelData.size()),
                                                 params);
    if (!m_ctx) {
        emit errorOccurred("加载模型失败: " + modelAssetPath);
        return false;
    }
    qDebug() << "Whisper 模型从内存加载成功，字节数" << modelData.size();
    return true;
}

void WhisperManager::feedAudioData(const int16_t *data, size_t sampleCount)
{
    std::vector<float> m_audioBuffer ;
    if (!data || sampleCount == 0) {
        emit errorOccurred("未收到有效音频样本");
        return;
    }

    for (size_t i = 0; i < sampleCount; ++i) {
        m_audioBuffer.push_back(static_cast<float>(data[i]) / 32768.0f);
    }

    qDebug() << "WhisperManager::feedAudioData sampleCount=" << sampleCount
             << "bufferSize=" << m_audioBuffer.size();
    emit errorOccurred(QString("已接收 %1 个音频样本，当前缓存 %2 个样本").arg(sampleCount).arg(m_audioBuffer.size()));

    if (m_audioBuffer.size() >= CHUNK_SIZE && !m_isProcessing) {
        m_isProcessing = true;

        auto audioChunk = std::move(m_audioBuffer);
        //m_audioBuffer.clear();
        qDebug() << "Start Whisper processing with" << audioChunk.size() << "samples";

        QFuture<void> future = QtConcurrent::run([this, audioChunk = std::move(audioChunk)]() {
            processBuffer(audioChunk);
        });
        m_futures.append(future);

        m_futures.erase(
            std::remove_if(m_futures.begin(), m_futures.end(),
                           [](QFuture<void> &f) { return f.isFinished(); }),
            m_futures.end());
    }
}

void WhisperManager::processBuffer(const std::vector<float> &audioChunk)
{
    if (!m_ctx || audioChunk.empty()) {
        emit errorOccurred("Whisper 收到空音频或模型未初始化");
        m_isProcessing = false;
        return;
    }

    qDebug() << "Whisper processBuffer samples=" << audioChunk.size();

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
    // system prompt 决定（API 路径见 msgsender.cpp:88，Ollama 路径见 :151，
    // 两处都是「你是智能眼镜上的语音助手，回答请简洁、口语化。」）。
    //
    // 即：输入英文 → whisper 转写成英文 → 大模型按中文 system prompt 回中文 →
    // 朗读中文。改动这一行会让转写质量下降，而原因不会在报错里出现。
    params.language = "en";
    params.offset_ms = 0;
    params.no_context = true ;
    params.single_segment = false;
    //排除空白token
    params.suppress_blank = true;
    params.suppress_nst = true;
    params.audio_ctx = 278 ;
    //解码与置信度阈值，防止幻觉
    params.temperature = 0.0f;
    params.temperature_inc = 0.2f;
    params.entropy_thold = 2.4f;
    params.logprob_thold = -1.0f;
    setpriority(PRIO_PROCESS, 0, -10);
    qDebug() << "Calling whisper_full with" << audioChunk.size() << "samples";
    //int result = whisper_full(m_ctx, params, audioChunk.data(), audioChunk.size());
    //qDebug() << "whisper_full returned"  ;
    //float *data = const_cast<float*>(audioChunk.data());
    //int size = audioChunk.size();
    qDebug() << "Whisper Data ready" ;
    qDebug() << "Whisper transcription began" ;
     if (whisper_full(m_ctx, params, const_cast<float*>(audioChunk.data()), audioChunk.size()) != 0) {
        //emit errorOccurred("转录失败，错误码: " + QString::number(result));
        emit errorOccurred("Whisper 转录失败");
        m_isProcessing = false;
        return;
    }
    else{
        qDebug() << "Whisper transcription completed successfully" ;
        whisper_print_timings(m_ctx);
    }

    QString fullText;
    int n_segments = whisper_full_n_segments(m_ctx);
    qDebug() << "Whisper segments=" << n_segments;
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(m_ctx, i);
        if (text) {
            fullText += QString::fromUtf8(text);
        }
    }

    qDebug() << "Whisper result text=" << fullText << "segments=" << n_segments;
    if (!fullText.isEmpty()) {
        emit transcriptionReady(fullText);
    } else {
        emit errorOccurred("Whisper 未识别到有效文本");
        emit transcriptionReady("[无识别结果]");
    }

    m_isProcessing = false;
}

void WhisperManager::processAudioFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("无法打开音频文件: " + filePath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        emit errorOccurred("音频文件为空");
        return;
    }

    std::vector<float> audioBuffer;
    const int16_t *pcmData = reinterpret_cast<const int16_t*>(data.constData());
    size_t sampleCount = static_cast<size_t>(data.size() / sizeof(int16_t));

    for (size_t i = 0; i < sampleCount; ++i) {
        audioBuffer.push_back(static_cast<float>(pcmData[i]) / 32768.0f);
    }

    qDebug() << "WhisperManager::processAudioFile sampleCount=" << sampleCount;

    if (audioBuffer.empty()) {
        emit errorOccurred("没有有效音频数据");
        return;
    }

    if (m_isProcessing) {
        emit errorOccurred("正在处理中，请稍后");
        return;
    }

    m_isProcessing = true;

    QFuture<void> future = QtConcurrent::run([this, audioBuffer]() {
        processBuffer(audioBuffer);
    });
    m_futures.append(future);

    m_futures.erase(
        std::remove_if(m_futures.begin(), m_futures.end(),
                       [](QFuture<void> &f) { return f.isFinished(); }),
        m_futures.end());
}

void WhisperManager::reset()
{
    m_audioBuffer.clear();
    m_isProcessing = false;
}