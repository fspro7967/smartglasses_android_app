#ifndef WHISPER_MANAGER_H
#define WHISPER_MANAGER_H

#include <QByteArray>
#include <QFuture>
#include <QObject>
#include <QString>

#include <atomic>
#include <deque>
#include <vector>

struct whisper_context;

// 离线语音识别（whisper.cpp）。
//
// 输入是一条连续的 16 kHz 单声道 int16 小端 PCM 流（蓝牙上行音频），输出是按段的
// 转写文本。本类自己负责三件事，调用方不需要也不应该再做：
//
//   1. **成段**：feedAudioData() 只管追加，凑满 kWindowSamples（3 秒）切成一段；
//      flush() 把不足一段的尾巴也成段（由调用方在「一段时间没有新数据」时触发）。
//      两个触发条件缺一不可——只按长度触发，最后不足 3 秒的话永远不会被识别。
//   2. **排队**：同一时刻只跑一次 whisper_full。上一段还没转写完时，新段进入
//      有界的待处理队列，而不是被丢掉；队列满了丢最早的一段并**明确上报**。
//   3. **就绪**：模型在后台线程加载（initAsync），isReady() 为假时一切输入都被
//      拒绝，而不是先收下、再在 worker 里发现模型没有。
//
// 线程模型：除 init() 与 processBuffer() 在 QtConcurrent 的 worker 线程里跑之外，
// 所有成员都只在对象所在线程（GUI 线程）读写，因此不需要锁。跨线程的只有三样：
// m_ready / m_epoch（原子），以及 m_ctx——它在 m_ready 置位之前写好，此后只读。
// worker 通过信号把结果投递回来（跨线程 emit 自动排队）。
class WhisperManager : public QObject
{
    Q_OBJECT
public:
    explicit WhisperManager(QObject *parent = nullptr);
    ~WhisperManager() override;

    // 在后台线程加载 assets 里的模型（例如 "assets:/models/model.bin"），立即返回。
    // 成功发 ready()，失败发 initFailed()。
    //
    // 内容整块读进内存后交给 whisper_init_from_buffer_with_params，**不解压到磁盘**：
    // 省掉首次运行 59.7 MB 的写入和一份重复存储，并从根上消除「进程被杀留下的半截
    // 文件被当成有效模型」的风险。whisper 在 init 调用期间就同步读完所有权重，所以
    // 读入的缓冲在返回后即释放；峰值内存约为「模型字节数 × 2」，初始化完成后回落。
    void initAsync(const QString &modelAssetPath);

    bool isReady() const { return m_ready.load(); }

    // 追加一段上行 PCM（任意长度，奇数字节也可以——半个样本会留到下一次）。
    // 未就绪时直接丢弃：调用方应先看 isReady()，并自行决定怎么提示用户。
    void feedAudioData(const QByteArray &pcm);

    // 把缓冲里不足一段的尾巴立即送去转写。短于 kMinFlushSamples 的尾巴直接丢弃
    // （太短的片段多半是噪声，喂给 whisper 只会得到幻觉文本）。
    void flush();

    // 读取一个音频文件并整体送去转写（WAV 会核对格式，见 wavpcm.h）。
    // 返回 false 表示没有受理，原因已经通过 errorOccurred() 给出。
    bool processAudioFile(const QString &filePath);

    // 丢弃所有尚未开始转写的音频，并作废正在转写的那一段的结果。
    // 断开或切换设备时调用：上一段会话的残留不能拼进下一段，也不该再产生回复。
    void reset();

    // 成段与冲刷的阈值（样本数）。
    static constexpr int kSampleRate      = 16000;
    static constexpr int kWindowSamples   = 3 * kSampleRate;   // 凑满 3 秒切一段
    static constexpr int kMinFlushSamples = kSampleRate / 2;   // 尾巴至少 0.5 秒才转写
    // 待处理队列上限：4 段 ≈ 12 秒音频，float 存储约 0.77 MB。
    static constexpr int kMaxPendingSegments = 4;

signals:
    void ready();
    void initFailed(const QString &error);
    // 一段音频已进入待处理队列。这是**进度**，不是错误，所以单独一个信号，
    // 不再借用 errorOccurred()。
    void segmentQueued(int sampleCount, int pendingSegments);
    void transcriptionReady(const QString &text);
    void errorOccurred(const QString &error);
    // 内部用：worker 结束一段转写后发出，在对象线程里推进下一段。
    void segmentFinished();

private:
    bool init(const QString &modelAssetPath, QString *errorOut);
    void appendSample(qint16 sample);
    void enqueueSegment(std::vector<float> &&segment);
    void startNextIfIdle();
    void processBuffer(const std::vector<float> &audio, quint32 epoch);

    whisper_context *m_ctx = nullptr;
    std::atomic_bool m_ready{false};
    // 每次 reset() 自增。worker 开工时记下当时的值，收工时不一致就不再上报结果。
    std::atomic<quint32> m_epoch{0};

    // ---- 以下只在对象线程读写 ----
    std::vector<float> m_window;                // 正在凑的一段
    bool m_haveCarry = false;                   // 上一次留下了半个样本
    char m_carryByte = 0;
    std::deque<std::vector<float>> m_pending;   // 已成段、等待转写
    bool m_busy = false;                        // 有一段正在 worker 里转写

    QFuture<void> m_initFuture;
    QFuture<void> m_workFuture;
};

#endif // WHISPER_MANAGER_H
