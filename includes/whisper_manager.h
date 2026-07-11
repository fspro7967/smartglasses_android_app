#ifndef WHISPER_MANAGER_H
#define WHISPER_MANAGER_H

#include <QObject>
#include <QFuture>
#include <vector>

struct whisper_context;

class WhisperManager : public QObject
{
    Q_OBJECT
public:
    explicit WhisperManager(QObject *parent = nullptr);
    ~WhisperManager();

    bool init(const QString &modelPath);
    void feedAudioData(const int16_t *data, size_t sampleCount);
    void reset();

signals:
    void transcriptionReady(const QString &text);
    void errorOccurred(const QString &error);

private:
    void processBuffer(const std::vector<float> &audioChunk);

    whisper_context *m_ctx = nullptr;
    std::vector<float> m_audioBuffer;
    bool m_isProcessing = false;
    QList<QFuture<void>> m_futures;

    static constexpr size_t CHUNK_SIZE = 3 * 16000;  // 3 秒 @16kHz
};

#endif // WHISPER_MANAGER_H