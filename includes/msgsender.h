#ifndef MSGSENDER_H
#define MSGSENDER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// 消息发送器：支持两种 AI 大模型接入方式
//   1. ProviderApi   : OpenAI 兼容的 Chat Completions 接口（需 API_BASE_URL / API_KEY / MODEL_NAME）
//   2. ProviderOllama: 本地部署的 Ollama（/api/chat 接口，需服务器地址与模型名）
// 配置通过 QSettings 持久化，可跨重启保留。
class MsgSender : public QObject
{
    Q_OBJECT
public:
    // AI 大模型接入方式
    enum Provider {
        ProviderApi = 0,    // OpenAI 兼容 API
        ProviderOllama = 1  // 本地 Ollama
    };
    Q_ENUM(Provider)

    explicit MsgSender(QObject *parent = nullptr);
    ~MsgSender() override;

    Provider provider() const { return m_provider; }

    // 切换接入方式并持久化
    void setProvider(Provider provider);
    // 配置 OpenAI 兼容 API（baseUrl 形如 https://api.deepseek.com/v1，不含 /chat/completions）
    void setApiConfig(const QString &baseUrl, const QString &apiKey, const QString &modelName);
    // 配置本地 Ollama（serverUrl 形如 http://192.168.1.100:11434）
    void setOllamaConfig(const QString &serverUrl, const QString &modelName);

    // 当前配置取值（供 UI 回显）
    QString apiBaseUrl() const { return m_apiBaseUrl; }
    QString apiKey() const { return m_apiKey; }
    QString apiModelName() const { return m_apiModel; }
    QString ollamaUrl() const { return m_ollamaUrl; }
    QString ollamaModelName() const { return m_ollamaModel; }

    // 异步发送消息至 AI 大模型，结果通过 responseReceived / errorOccurred 信号返回
    void sendMessage(const QString &message);

signals:
    void messageSent(const QString &message);
    void responseReceived(const QString &response);
    void errorOccurred(const QString &error);

private:
    void sendToApi(const QString &message);
    void sendToOllama(const QString &message);
    void handleApiReply(QNetworkReply *reply, const QString &message);
    void handleOllamaReply(QNetworkReply *reply, const QString &message);

    QNetworkAccessManager *m_networkManager;

    // 当前接入方式
    Provider m_provider = ProviderApi;

    // OpenAI 兼容 API 配置
    QString m_apiBaseUrl;
    QString m_apiKey;
    QString m_apiModel;

    // 本地 Ollama 配置
    QString m_ollamaUrl;
    QString m_ollamaModel;
};

#endif // MSGSENDER_H
