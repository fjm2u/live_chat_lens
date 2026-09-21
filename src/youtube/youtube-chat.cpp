#include "youtube-chat.hpp"
#include <QDateTime>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
namespace comments {
QString videoId(const QString &input) {
    static const QRegularExpression valid("^[A-Za-z0-9_-]{11}$");
    const auto text=input.trimmed();
    if (valid.match(text).hasMatch()) return text;
    const QUrl url(text);
    if (url.scheme()!="https" && url.scheme()!="http") return {};
    const auto host=url.host().toLower();
    QString id;
    if (host=="youtu.be") id=url.path().section('/',1,1);
    else if (host=="youtube.com" || host=="www.youtube.com" || host=="m.youtube.com") {
        if (url.path()=="/watch") id=QUrlQuery(url).queryItemValue("v");
        else if (url.path().startsWith("/live/")) id=url.path().section('/',2,2);
    }
    return valid.match(id).hasMatch()?id:QString();
}
std::vector<Comment> chatComments(const QJsonObject &root, Millis received) {
    std::vector<Comment> result;
    for (const auto &item:root.value("items").toArray()) {
        const auto object=item.toObject(), snippet=object.value("snippet").toObject();
        const auto type=snippet.value("type").toString();
        if (type!="textMessageEvent" && type!="superChatEvent") continue;
        auto text=snippet.value("displayMessage").toString().normalized(QString::NormalizationForm_C).trimmed();
        if (text.size()>500) continue;
        result.push_back({object.value("id").toString().toStdString(),object.value("authorDetails").toObject().value("channelId").toString().toStdString(),text.toStdString(),received,
            QDateTime::fromString(snippet.value("publishedAt").toString(),Qt::ISODateWithMs).toMSecsSinceEpoch()});
    }
    return result;
}
}
