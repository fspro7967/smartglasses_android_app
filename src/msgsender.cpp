#include "msgsender.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QSettings>
#include <QDebug>

// ==================== 默认配置 ====================
// OpenAI 兼容 API（示例：DeepSeek）
static const QString kDefaultApiBaseUrl = "";
static const QString kDefaultApiModel  = "";
static const QString kDefaultApiKey    = "";
// 本地 Ollama
static const QString kDefaultOllamaUrl   = "";
static const QString kDefaultOllamaModel = "";

MsgSender::MsgSender(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // 从持久化配置加载（首次启动时使用默认值）
    QSettings s;
    m_provider     = static_cast<Provider>(s.value("ai/provider", int(ProviderApi)).toInt());
    m_apiBaseUrl   = s.value("ai/apiBaseUrl", kDefaultApiBaseUrl).toString();
    m_apiKey       = s.value("ai/apiKey", kDefaultApiKey).toString();
    m_apiModel     = s.value("ai/apiModel", kDefaultApiModel).toString();
    m_ollamaUrl    = s.value("ai/ollamaUrl", kDefaultOllamaUrl).toString();
    m_ollamaModel  = s.value("ai/ollamaModel", kDefaultOllamaModel).toString();
}

MsgSender::~MsgSender() = default;

void MsgSender::setProvider(Provider provider)
{
    m_provider = provider;
    QSettings s;
    s.setValue("ai/provider", int(provider));
}

void MsgSender::setApiConfig(const QString &baseUrl, const QString &apiKey, const QString &modelName)
{
    m_apiBaseUrl = baseUrl.trimmed();
    m_apiKey = apiKey.trimmed();
    m_apiModel = modelName.trimmed();

    QSettings s;
    s.setValue("ai/apiBaseUrl", m_apiBaseUrl);
    s.setValue("ai/apiKey", m_apiKey);
    s.setValue("ai/apiModel", m_apiModel);
}

void MsgSender::setOllamaConfig(const QString &serverUrl, const QString &modelName)
{
    m_ollamaUrl = serverUrl.trimmed();
    m_ollamaModel = modelName.trimmed();

    QSettings s;
    s.setValue("ai/ollamaUrl", m_ollamaUrl);
    s.setValue("ai/ollamaModel", m_ollamaModel);
}

void MsgSender::sendMessage(const QString &message)
{
    if (message.trimmed().isEmpty()) {
        emit errorOccurred("消息为空，未发送");
        return;
    }

    if (m_provider == ProviderOllama)
        sendToOllama(message);
    else
        sendToApi(message);
}

// ==================== OpenAI 兼容 API 接入 ====================
void MsgSender::sendToApi(const QString &message)
{
    QNetworkRequest request(QUrl(m_apiBaseUrl + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "你是智能眼镜上的语音助手，回答请简洁、口语化。";

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = message;

    QJsonObject payload;
    payload["model"] = m_apiModel;
    payload["messages"] = QJsonArray{systemMsg, userMsg};
    payload["stream"] = false;

    qDebug() << "MsgSender: 发送 API 请求至" << m_apiBaseUrl << "模型:" << m_apiModel;

    QNetworkReply *reply = m_networkManager->post(
        request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, message]() {
        handleApiReply(reply, message);
    });
}

void MsgSender::handleApiReply(QNetworkReply *reply, const QString &message)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("AI 请求失败: " + reply->errorString());
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred("AI 响应解析失败: " + parseError.errorString());
        return;
    }

    const QJsonArray choices = doc.object().value("choices").toArray();
    if (choices.isEmpty()) {
        emit errorOccurred("AI 响应缺少 choices 字段");
        return;
    }

    const QString content = choices.first().toObject()
                                .value("message").toObject()
                                .value("content").toString();
    if (content.trimmed().isEmpty()) {
        emit errorOccurred("AI 响应内容为空");
        return;
    }

    emit messageSent(message);
    emit responseReceived(content);
}

// ==================== 本地 Ollama 接入 ====================
void MsgSender::sendToOllama(const QString &message)
{
    QNetworkRequest request(QUrl(m_ollamaUrl + "/api/chat"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "你是智能眼镜上的语音助手，回答请简洁、口语化。";

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = message;

    QJsonObject payload;
    payload["model"] = m_ollamaModel;
    payload["messages"] = QJsonArray{systemMsg, userMsg};
    payload["stream"] = false;

    qDebug() << "MsgSender: 发送 Ollama 请求至" << m_ollamaUrl << "模型:" << m_ollamaModel;

    QNetworkReply *reply = m_networkManager->post(
        request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, message]() {
        handleOllamaReply(reply, message);
    });
}

void MsgSender::handleOllamaReply(QNetworkReply *reply, const QString &message)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Ollama 请求失败: " + reply->errorString());
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred("Ollama 响应解析失败: " + parseError.errorString());
        return;
    }

    const QString content = doc.object().value("message").toObject()
                                .value("content").toString();
    if (content.trimmed().isEmpty()) {
        emit errorOccurred("Ollama 响应内容为空");
        return;
    }

    emit messageSent(message);
    emit responseReceived(content);
}
