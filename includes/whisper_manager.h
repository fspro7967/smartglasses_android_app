#ifndef WHISPER_MANAGER_H
#define WHISPER_MANAGER_H

#include <QObject>
#include <QFuture>
#include <vector>
#include <sys/resource.h>

struct whisper_context;

class WhisperManager : public QObject
{
    Q_OBJECT
public:
    explicit WhisperManager(QObject *parent = nullptr);
    ~WhisperManager();

    // 从 assets 中的模型文件加载（例如 "assets:/models/model.bin"）。
    //
    // 内容整块读进内存后交给 whisper_init_from_buffer_with_params，**不再解压到
    // 磁盘**：省掉首次运行 59.7 MB 的写入和一份重复存储，并从根上消除「进程被杀
    // 留下的半截文件被当成有效模型」的风险。
    //
    // 注意：whisper 在 init 调用期间就同步读完所有权重（其内部 loader 的
    // buf_context 分配在栈上），所以读入的 QByteArray 在返回后即可释放。
    // 峰值内存约为「模型字节数 × 2」的瞬时占用，初始化完成后回落。
    bool init(const QString &modelAssetPath);
    void feedAudioData(const int16_t *data, size_t sampleCount);
    void processAudioFile(const QString &filePath);
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

    static constexpr size_t CHUNK_SIZE = 16000;
};

#endif // WHISPER_MANAGER_H