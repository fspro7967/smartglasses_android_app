//#ifndef MAINWINDOW_H
//#define MAINWINDOW_H

#include <QObject>
#include <QBluetoothDeviceInfo>
#include <QList>
#include <QByteArray>
#include "devicehandler.h"
#include "whisper_manager.h"
#include "msgsender.h"

// MainWindow 作为 QML 后端桥接类：所有 UI 交互由 QML 调用，
// 业务逻辑（蓝牙、Whisper 识别、AI 对话）通过信号暴露给 QML。
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

    Q_INVOKABLE void startScan();
    Q_INVOKABLE void connectToDevice(int index);
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void enableNotification(const QString &serviceUuid, const QString &charUuid);
    Q_INVOKABLE void processAudioFile(const QString &filePath);
    Q_INVOKABLE void sendMessageToServer(const QString &message);
    // 设置 AI 回复的 BLE 写入目标（须为可写特征）
    Q_INVOKABLE void setWriteTarget(const QString &serviceUuid, const QString &charUuid);

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

private:
    void requestAndroidPermissions();
    void clearServices();
    void clearWriteTarget();          // 清空 BLE 写入目标（断开/切换设备时调用）
    QString extractModelToFile();

    DeviceHandler *m_deviceHandler;
    MsgSender *m_msgsender;
    WhisperManager *m_whisperManager = nullptr;
    QList<QBluetoothDeviceInfo> m_discoveredDevices;
    QVariantList m_services;            // 服务/特征树数据（暴露给 QML）
    QByteArray m_audioBuffer;
    QString m_modelPath;
    bool m_isScanning = false;
    bool m_isConnected = false;
    QString m_deviceName;
    QString m_pendingDeviceName; // 正在连接中的设备名（连接成功时使用）
    QString m_writeServiceUuid;  // BLE 写入目标服务 UUID
    QString m_writeCharUuid;     // BLE 写入目标特征 UUID
};

//#endif // MAINWINDOW_H
