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

#include <chrono>

using namespace std::chrono_literals;

// ==================== 默认配置 ====================
// OpenAI 兼容 API（示例：DeepSeek）
static const QString kDefaultApiBaseUrl = "";
static const QString kDefaultApiModel  = "";
static const QString kDefaultApiKey    = "";
// 本地 Ollama
static const QString kDefaultOllamaUrl   = "";
static const QString kDefaultOllamaModel = "";

// 两条接入方式共用的 system prompt。回复之所以是中文，就是由它决定的
// （转写固定是英文，见 whisper_manager.cpp 里对 params.language 的说明，
// 以及 docs/adr/0002-english-in-chinese-out.md）。
static const QString kSystemPrompt = "你是智能眼镜上的语音助手，回答请简洁、口语化。";

namespace {

// 请求的「无数据传输」超时：这么久一个字节都没有往来就中止，走 errorOccurred。
//
// 没有它时，服务器接受连接却不回包（地址填错、代理吞包、TCP 半开），finished
// 可能迟迟不来：界面既不成功也不失败，使用者只会以为应用卡死了。
//
// 两个值不同是有意的。请求是非流式的（stream=false），回复生成完之前服务器一个
// 字节都不发，所以这个超时同时也是「最长允许思考多久」：
//   * 云端 API：15 秒。语音助手等这么久已经是一次失败的交互。
//   * 本地 Ollama：60 秒。首个请求要先把模型载入内存，冷启动几十秒很常见；
//     按 15 秒掐，每次开机后的第一句话都会被误判为超时。
constexpr auto kApiTimeout    = 15s;
constexpr auto kOllamaTimeout = 60s;

// 去掉结尾的斜杠，避免拼出 "https://host/v1//chat/completions"。
QString trimTrailingSlashes(QString url)
{
    while (url.endsWith(QLatin1Char('/')))
        url.chop(1);
    return url;
}

bool isHttpUrl(const QString &url)
{
    const QUrl parsed(url);
    return parsed.isValid() && !parsed.host().isEmpty()
        && (parsed.scheme() == QLatin1String("http")
            || parsed.scheme() == QLatin1String("https"));
}

// 从错误响应的正文里取出服务端给的原因。只认两种已知结构里的那一个字段
// （OpenAI 兼容：{"error":{"message":"..."}}；Ollama：{"error":"..."}），
// 不把整段正文原样显示——正文里可能带请求头回显等不该上屏的内容。
QString serverErrorMessage(const QByteArray &body)
{
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject())
        return QString();
    const QJsonValue error = doc.object().value("error");
    QString message = error.isObject() ? error.toObject().value("message").toString()
                                       : error.toString();
    message = message.simplified();
    if (message.size() > 200)
        message = message.left(200) + QStringLiteral("…");
    return message;
}

// 把一次失败归到三类之一，让状态栏上的话能直接指向该查哪里：
// 超时 / 服务端返回了错误 / 根本没连上。
QString describeFailure(QNetworkReply *reply)
{
    const QNetworkReply::NetworkError error = reply->error();

    // 传输超时（setTransferTimeout 触发）。本类从不主动 abort()，所以
    // OperationCanceledError 在这里也只可能来自超时。
    if (error == QNetworkReply::TimeoutError
            || error == QNetworkReply::OperationCanceledError) {
        return QStringLiteral("请求超时，服务器长时间没有响应");
    }

    // 拿到了 HTTP 状态码：连上了，是服务端拒绝或出错（401/404/429/500…）。
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status > 0) {
        const QString detail = serverErrorMessage(reply->readAll());
        return detail.isEmpty()
            ? QStringLiteral("服务端返回错误 HTTP %1").arg(status)
            : QStringLiteral("服务端返回错误 HTTP %1: %2").arg(status).arg(detail);
    }

    // 没有状态码：请求没到达服务端（DNS、拒绝连接、TLS、断网…）。
    return QStringLiteral("连不上服务器: ") + reply->errorString();
}

} // namespace

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
    // 先核对配置，缺了就明说，而不是拿空地址发一个注定失败的请求、
    // 再让使用者从一条网络错误里猜原因。
    const QString baseUrl = trimTrailingSlashes(m_apiBaseUrl);
    if (baseUrl.isEmpty() || m_apiKey.isEmpty() || m_apiModel.isEmpty()) {
        emit errorOccurred("尚未配置 API 接入（地址 / 密钥 / 模型），请在设置里填写");
        return;
    }
    if (!isHttpUrl(baseUrl)) {
        emit errorOccurred("API 地址无效，应以 http:// 或 https:// 开头: " + m_apiBaseUrl);
        return;
    }

    QNetworkRequest request(QUrl(baseUrl + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    request.setTransferTimeout(kApiTimeout);

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = kSystemPrompt;

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
        emit errorOccurred("AI 请求失败: " + describeFailure(reply));
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
    const QString serverUrl = trimTrailingSlashes(m_ollamaUrl);
    if (serverUrl.isEmpty() || m_ollamaModel.isEmpty()) {
        emit errorOccurred("尚未配置 Ollama 接入（服务器地址 / 模型），请在设置里填写");
        return;
    }
    if (!isHttpUrl(serverUrl)) {
        emit errorOccurred("Ollama 地址无效，应以 http:// 或 https:// 开头: " + m_ollamaUrl);
        return;
    }

    QNetworkRequest request(QUrl(serverUrl + "/api/chat"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(kOllamaTimeout);

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = kSystemPrompt;

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
        emit errorOccurred("Ollama 请求失败: " + describeFailure(reply));
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
