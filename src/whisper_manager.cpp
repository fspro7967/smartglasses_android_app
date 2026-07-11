#include "whisper_manager.h"
#include "whisper.h"
#include <QDebug>
#include <QtConcurrent>
#include <algorithm>

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

bool WhisperManager::init(const QString &modelPath)
{
    m_ctx = whisper_init_from_file(modelPath.toUtf8().constData());
    if (!m_ctx) {
        emit errorOccurred("加载模型失败: " + modelPath);
        return false;
    }
    qDebug() << "Whisper 模型加载成功";
    return true;
}

void WhisperManager::feedAudioData(const int16_t *data, size_t sampleCount)
{
    for (size_t i = 0; i < sampleCount; ++i) {
        m_audioBuffer.push_back(data[i] / 32768.0f);
    }

    if (m_audioBuffer.size() >= CHUNK_SIZE && !m_isProcessing) {
        m_isProcessing = true;

        auto audioChunk = std::move(m_audioBuffer);

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
        m_isProcessing = false;
        return;
    }

    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = 4;
    params.language = "auto";        // 改为自动检测，支持中英混合

    int result = whisper_full(m_ctx, params, audioChunk.data(), audioChunk.size());
    if (result != 0) {
        emit errorOccurred("转录失败，错误码: " + QString::number(result));
        m_isProcessing = false;
        return;
    }

    QString fullText;
    int n_segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(m_ctx, i);
        if (text) {
            fullText += QString::fromUtf8(text);
        }
    }

    if (!fullText.isEmpty()) {
        emit transcriptionReady(fullText);
    }

    m_isProcessing = false;
}

void WhisperManager::reset()
{
    m_audioBuffer.clear();
    m_isProcessing = false;
}