#ifndef MSGSENDER_H
#define MSGSENDER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// 消息发送器：通过 OpenAI 兼容的 Chat Completions 接口将文本发送至 AI 大模型
class MsgSender : public QObject
{
    Q_OBJECT
public:
    explicit MsgSender(QObject *parent = nullptr);
    ~MsgSender() override;

    // 异步发送消息至 AI 大模型，结果通过 responseReceived / errorOccurred 信号返回
    void sendMessage(const QString &message);

signals:
    void messageSent(const QString &message);
    void responseReceived(const QString &response);
    void errorOccurred(const QString &error);

private:
    void handleReply(QNetworkReply *reply, const QString &message);

    QNetworkAccessManager *m_networkManager;

    // ==================== AI 大模型 API 配置（OpenAI 兼容格式） ====================
    // DeepSeek:   https://api.deepseek.com/v1             模型: deepseek-chat
    // OpenAI:     https://api.openai.com/v1              模型: gpt-4o-mini
    // 通义千问:    https://dashscope.aliyuncs.com/compatible-mode/v1  模型: qwen-plus
    static const QString API_BASE_URL;
    static const QString API_KEY;
    static const QString MODEL_NAME;
};

#endif // MSGSENDER_H
