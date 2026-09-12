//#ifndef MAINWINDOW_H
//#define MAINWINDOW_H

#include <QObject>
#include <QBluetoothDeviceInfo>
#include <QFuture>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include "devicehandler.h"
#include "whisper_manager.h"
#include "msgsender.h"
#include "speechsynthesizer.h"

class QAudioSink;
class QIODevice;
class QTimer;

// MainWindow 作为 QML 后端桥接类：所有 UI 交互由 QML 调用，
// 业务逻辑（蓝牙、Whisper 识别、AI 对话、TTS 朗读）通过信号暴露给 QML。
class MainWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY connectionChanged)
    // 服务/特征树模型：每项为 { type, serviceUuid, label, chars: [{ serviceUuid, charUuid, charName, props, notifiable }] }
    Q_PROPERTY(QVariantList services READ services NOTIFY servicesChanged)
    // BLE 写入目标（AI 回复回传眼镜）：格式 "serviceUuid:charUuid"，未设置时为空字符串
    Q_PROPERTY(QString writeTarget READ writeTarget NOTIFY writeTargetChanged)
    // 写入目标特征 UUID（仅用于界面展示，更易读）
    Q_PROPERTY(QString writeTargetName READ writeTargetName NOTIFY writeTargetChanged)
    // 语音合成是否可用（模型已就绪 + 音频输出已打开）。QML 用它决定是否可用朗读按钮。
    Q_PROPERTY(bool ttsReady READ isTtsReady NOTIFY ttsReadyChanged)
    // 是否正在朗读。QML 用它决定「停止」按钮的可用状态。
    Q_PROPERTY(bool speaking READ isSpeaking NOTIFY speakingChanged)

public:
    explicit MainWindow(QObject *parent = nullptr);
    ~MainWindow() override;

    bool isScanning() const { return m_isScanning; }
    bool isConnected() const { return m_isConnected; }
    QString deviceName() const { return m_deviceName; }
    QVariantList services() const { return m_services; }
    QString writeTarget() const {
        return m_writeServiceUuid.isEmpty()
                ? QString()
                : m_writeServiceUuid + ":" + m_writeCharUuid;
    }
    QString writeTargetName() const { return m_writeCharUuid; }
    bool isTtsReady() const { return m_ttsReady; }
    bool isSpeaking() const { return m_speaking; }

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void connectToDevice(int index);
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void enableNotification(const QString &serviceUuid, const QString &charUuid);
    Q_INVOKABLE void processAudioFile(const QString &filePath);
    Q_INVOKABLE void sendMessageToServer(const QString &message);
    // 设置 AI 回复的 BLE 写入目标（须为可写特征）
    Q_INVOKABLE void setWriteTarget(const QString &serviceUuid, const QString &charUuid);
    // 立即停止朗读并丢弃已缓冲音频
    Q_INVOKABLE void stopSpeaking();
    // 重新朗读上一条 AI 回复（回复为空时什么也不做）
    Q_INVOKABLE void replayLastReply();

    // ===== AI 大模型接入方式（暴露给 QML 的设置菜单） =====
    Q_INVOKABLE int aiProvider() const;                            // 0=API, 1=Ollama
    Q_INVOKABLE QString aiProviderName() const;                    // 当前接入方式名称
    Q_INVOKABLE QString apiBaseUrl() const;
    Q_INVOKABLE QString apiKey() const;
    Q_INVOKABLE QString apiModelName() const;
    Q_INVOKABLE QString ollamaUrl() const;
    Q_INVOKABLE QString ollamaModelName() const;
    Q_INVOKABLE void setAiProvider(int provider);
    Q_INVOKABLE void setApiConfig(const QString &baseUrl, const QString &apiKey, const QString &modelName);
    Q_INVOKABLE void setOllamaConfig(const QString &serverUrl, const QString &modelName);

signals:
    void deviceDiscovered(const QString &name);                       // 发现新设备
    void statusMessage(const QString &msg);                           // 状态/日志消息
    void servicesChanged();                                           // 服务/特征模型已更新
    void transcriptionReady(const QString &text);                     // Whisper 识别结果
    void aiResponseReady(const QString &response);                    // AI 大模型回复
    void scanningChanged();
    void connectionChanged();
    void aiConfigChanged();                                           // AI 接入方式/配置已变更
    void writeTargetChanged();                                        // BLE 写入目标已变更
    void ttsReadyChanged();
    void speakingChanged();
    // 内部用：把后台线程的提取/初始化结果投递回 GUI 线程（见 prepareTtsModel）
    void ttsPrepared(int sampleRate);
    void ttsPrepareFailed(const QString &error);

private slots:
    void onDataReceived(const QByteArray &data);
    void onServiceDiscovered(const QString &serviceUuid);
    void onCharacteristicDiscovered(const QString &serviceUuid,
                                    const QString &charUuid,
                                    const QString &charName,
                                    int properties);
    void onTranscriptionResult(const QString &text);
    void onAIResponse(const QString &response);
    void onWhisperError(const QString &error);
    void onBluetoothPermissionGranted();
    // TTS 模型提取 + 合成器初始化完成（在 GUI 线程接收）
    void onTtsPrepared(int sampleRate);
    void onTtsPrepareFailed(const QString &error);
    void onAudioChunk(const QByteArray &pcm, int sampleRate);
    void onSentenceFinished();
    void onSpeechFailed(const QString &reason);

private:
    void requestAndroidPermissions();
    void clearServices();
    void clearWriteTarget();          // 清空 BLE 写入目标（断开/切换设备时调用）
    // 在后台线程提取 TTS 模型并初始化合成器（绝不占用构造函数）
    void prepareTtsModel();
    // 按模型采样率打开 QAudioSink；设备不支持时退回设备采样率并自行重采样
    void setupAudioOutput(int modelSampleRate);
    // 把暂存的 PCM 尽量写进音频设备
    void writePendingAudio();
    void stopPlayback();
    // 把一段回复切句后按「一句合成完再喂下一句」的节奏朗读
    void speakReply(const QString &reply);
    void speakNextSentence();
    // 三条件全满足才算念完：句子全部喂出、全部合成完毕、音频全部交给设备
    void maybeFinishSpeaking();
    // 朗读状态的唯一写入口，避免「改标志 + 发信号」这套动作散落多处
    void setSpeaking(bool speaking);

    DeviceHandler *m_deviceHandler;
    MsgSender *m_msgsender;
    WhisperManager *m_whisperManager = nullptr;
    SpeechSynthesizer *m_synthesizer = nullptr;
    QList<QBluetoothDeviceInfo> m_discoveredDevices;
    QVariantList m_services;            // 服务/特征树数据（暴露给 QML）
    QByteArray m_audioBuffer;
    bool m_isScanning = false;
    bool m_isConnected = false;
    QString m_deviceName;
    QString m_pendingDeviceName; // 正在连接中的设备名（连接成功时使用）
    QString m_writeServiceUuid;  // BLE 写入目标服务 UUID
    QString m_writeCharUuid;     // BLE 写入目标特征 UUID

    // ===== 朗读（TTS）状态 =====
    QAudioSink *m_audioSink = nullptr;
    QIODevice *m_audioDevice = nullptr;
    // 独立的排空定时器。**必须存在**：音频设备以真实速率消耗，而合成快好几倍，
    // 只在「收到新音频块」时才写的话，合成一停下来尾部就永远写不出去了。
    QTimer *m_audioPump = nullptr;
    QByteArray m_pendingPcm;            // 设备一时写不进去时暂存
    int m_audioSampleRate = 0;          // 实际喂给设备的采样率
    bool m_needResample = false;        // 设备不支持模型采样率时置位
    QStringList m_speakQueue;           // 本次回复切出的句子
    int m_speakIndex = 0;
    int m_sentencesOutstanding = 0;     // 已喂给合成器但还没合成完的句子数
    QString m_lastReply;                // 供「重播」使用
    bool m_ttsReady = false;
    bool m_ttsPreparing = false;
    bool m_speaking = false;
    // 连续「有积压但一个字节都写不进去」的排空次数，用于识别设备无响应
    int m_stalledTicks = 0;
    QList<QFuture<void>> m_ttsFutures;  // 持有后台提取任务的句柄
};

//#endif // MAINWINDOW_H
