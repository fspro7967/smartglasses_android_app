#include "msgsender.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

// ==================== AI 大模型 API 配置（OpenAI 兼容格式） ====================
// 请将 API_KEY 替换为你自己的密钥
const QString MsgSender::API_BASE_URL = "https://api.deepseek.com/v1";
const QString MsgSender::API_KEY = "";
const QString MsgSender::MODEL_NAME = "deepseek-v4-flash";

MsgSender::MsgSender(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

MsgSender::~MsgSender() = default;

void MsgSender::sendMessage(const QString &message)
{
    if (message.trimmed().isEmpty()) {
        emit errorOccurred("消息为空，未发送");
        return;
    }

    QNetworkRequest request(QUrl(API_BASE_URL + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + API_KEY.toUtf8());

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "你是智能眼镜上的语音助手，回答请简洁、口语化。";

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = message;

    QJsonObject payload;
    payload["model"] = MODEL_NAME;
    payload["messages"] = QJsonArray{systemMsg, userMsg};
    payload["stream"] = false;

    qDebug() << "MsgSender: 发送请求至" << API_BASE_URL << "内容:" << message;

    QNetworkReply *reply = m_networkManager->post(
        request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, message]() {
        handleReply(reply, message);
    });
}

void MsgSender::handleReply(QNetworkReply *reply, const QString &message)
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
