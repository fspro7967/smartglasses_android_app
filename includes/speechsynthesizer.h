#ifndef SPEECHSYNTHESIZER_H
#define SPEECHSYNTHESIZER_H

#include <QByteArray>
#include <QFuture>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <cstdint>

struct SherpaOnnxOfflineTts;
class QTimer;

// 离线中文语音合成（sherpa-onnx + vits-icefall-zh-aishell3），与 WhisperManager 对称。
//
// 接口纪律 —— 这是硬要求，因为 WhisperManager 在同样三点上做错了，照抄会重犯：
//
//   1. **静默是一种正常结果，不是错误。** 文本为空/纯空白时 synthesizeSentence()
//      返回 false 且**不发 failed**。对照 whisper_manager.cpp:134-135，那里把
//      「无识别结果」同时报成 errorOccurred 和一个文字哨兵值，逼调用方用字符串
//      比较去判断正常路径。
//   2. **进度不走错误通道。** 本类没有任何进度信号——界面看 audioChunk 即可。
//      对照 whisper_manager.cpp:49，那里用 errorOccurred 上报日常喂入进度。
//   3. **init 的结果必须被消费。** ready()/isReady() 让调用方能**拒绝**在未就绪
//      时调用合成。对照 mainwindow.cpp:63-67，那里拿到 init 失败后仍继续接活，
//      错误随后按块重复涌现。
//
// 线程模型：synthesizeSentence() 立即返回；合成在 QtConcurrent 的 worker 线程里
// 跑（与仓库既有用法一致）。sherpa 的增量回调把 PCM 写进一个**有上限**的字节
// 队列，对象所在线程的 QTimer 把它排空并通过 audioChunk 发出——所以队列有界，
// 且信号在正确的线程上发出。
//
// 同一时刻**只会有一次** sherpa 合成在跑，绝不并发使用同一个 tts 句柄。
// 取消是按代次（generation）记账的：stop() 把当前代次标记为「及此为止全部取消」，
// 因此 stop() 之后立刻开始下一句，也不会让上一句的 worker 复活。
class SpeechSynthesizer : public QObject
{
    Q_OBJECT
public:
    explicit SpeechSynthesizer(QObject *parent = nullptr);
    ~SpeechSynthesizer() override;

    // 加载模型。同步阻塞（要解析 29 MB 的 ONNX），调用方必须放到后台线程执行。
    // 失败返回 false 并发 failed()，此后 isReady() 恒为 false。
    bool init(const QString &modelDir);

    bool isReady() const { return m_tts != nullptr; }

    // 模型真实采样率（vits-icefall-zh-aishell3 = 8000 Hz）。未就绪时为 0。
    int sampleRate() const { return m_sampleRate; }

    // 排队合成一句。返回 false 表示**没有排队**——未就绪或文本为空白。
    // 两者都不算错误，不会发出 failed()。
    // 成功排队后按顺序合成，每句完成后发一次 sentenceFinished()。
    bool synthesizeSentence(const QString &text);

    // 丢弃当前合成与全部排队文本，并清空待播 PCM。
    // 用户主动停止不是错误，因此**不发 failed()**。
    void stop();

signals:
    // 增量 PCM：int16 单声道小端。sampleRate 与 QAudioFormat 对应。
    void audioChunk(const QByteArray &pcm, int sampleRate);
    // 一句合成完且其音频已全部发出。
    void sentenceFinished();
    // 真正的失败：模型未加载、sherpa 返回空、播放队列溢出。
    // 正常路径（含静默与用户主动停止）不会走到这里。
    void failed(const QString &reason);

private:
    bool hasPending() const;
    void enqueue(const QString &text);
    void tryStartWorker();
    void drainQueue();
    void finishCurrentSentence();
    bool appendPcm(const float *samples, int32_t n);

    // sherpa 的增量回调（C 函数指针，故为静态成员；真实实现转发到 appendPcm）
    static int32_t onGeneratedAudio(const float *samples, int32_t n,
                                    float progress, void *arg);
    // worker 线程入口
    static void generateInWorker(SpeechSynthesizer *self, QString sentence,
                                 quint64 generationId);

    const SherpaOnnxOfflineTts *m_tts = nullptr;
    int m_sampleRate = 0;

    // 模型路径必须活到 SherpaOnnxCreateOfflineTts() 返回之后才安全；
    // 存成成员，生命周期覆盖整个对象。
    QByteArray m_modelFile;
    QByteArray m_lexiconFile;
    QByteArray m_tokensFile;
    QByteArray m_ruleFsts;

    // 排队中的句子（对象线程写，drain 读）
    mutable QMutex m_pendingMutex;
    QStringList m_pending;

    // 待发 PCM（worker 写，drain 读）
    mutable QMutex m_pcmMutex;
    QByteArray m_pcmQueue;

    QTimer *m_drainTimer = nullptr;
    QFuture<void> m_future;

    // 同一时刻只允许一个 worker 在用 tts 句柄
    std::atomic_bool m_generating{false};
    // 当前代次；每开始一句自增
    std::atomic<quint64> m_generationId{0};
    // 所有 <= 该代次的任务都已取消。stop() 只抬高它，从不回落，
    // 因此被取消的 worker 永远不会因为「标志被重置」而复活。
    std::atomic<quint64> m_cancelUpTo{0};
    // 本次生成是否已收尾（worker 置位，drain 消费）
    std::atomic_bool m_generationDone{false};
    // 本次生成是否真的失败 / 是否发生队列溢出（worker 置位，drain 消费并上报）
    std::atomic_bool m_generateFailed{false};
    std::atomic_bool m_overflowed{false};
};

#endif // SPEECHSYNTHESIZER_H
