#include "engine.hpp"
#include "credential-store.hpp"
#include "jev/jev-client.hpp"
#include "youtube/youtube-chat.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QDir>
#include <QUrlQuery>
#include <algorithm>
namespace comments {
static QNetworkRequest request(const QUrl &url) {
    QNetworkRequest req(url);
    req.setTransferTimeout(5000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    return req;
}
void Engine::updateEvaluationPrompt(QString prompt) { settings_.evaluationPrompt=std::move(prompt); }
void Engine::restoreJevKey() {
    const auto result=loadJevKey();
    emit jevKeyRestored(result.key); emit credentialStatus(result.error);
}
void Engine::storeJevKey(QString key) {
    emit credentialStatus(saveJevKey(key.trimmed()));
}
void Engine::start(Settings settings) {
    stop(); settings_=std::move(settings); running_=true;
    if (!network_) network_=new QNetworkAccessManager(this);
    poll_=new QTimer(this); poll_->setSingleShot(true);
    connect(poll_,&QTimer::timeout,this,&Engine::pollYoutube);
    batchTimer_=new QTimer(this); batchTimer_->setInterval(150);
    connect(batchTimer_,&QTimer::timeout,this,&Engine::flush); batchTimer_->start();
    QDir().mkpath(settings_.logDirectory);
    log_.setFileName(settings_.logDirectory+"/events.jsonl");
    if (!log_.open(QIODevice::WriteOnly|QIODevice::Append)) emit logStatus("ログを保存できません");
    else { log_.setPermissions(QFile::ReadOwner|QFile::WriteOwner); emit logStatus(""); }
    emit jevStatus("Jev: waiting");
    if (settings_.youtubeKey.startsWith("oauth:")) {
        QFile file(settings_.youtubeKey.mid(6));
        if (!file.open(QIODevice::ReadOnly) || file.size()>65536) { emit youtubeStatus("YouTube: OAuthファイルを読み込めません"); return; }
        oauth_=QJsonDocument::fromJson(file.readAll()).object();
        for (const auto field:{"client_id","client_secret","refresh_token"}) {
            if (oauth_.value(field).toString().isEmpty()) { oauth_={}; emit youtubeStatus("YouTube: OAuthファイルが不正です"); return; }
        }
    }
    pollYoutube();
}
void Engine::stop() {
    running_=false; ++generation_;
    if (poll_) { delete poll_; poll_=nullptr; }
    if (batchTimer_) { delete batchTimer_; batchTimer_=nullptr; }
    const auto replies=replies_; replies_.clear();
    for (auto *reply:replies) { reply->disconnect(this); reply->abort(); reply->deleteLater(); }
    pending_.clear(); filter_=BasicFilter{}; seen_.clear(); seenOrder_.clear();
    chatId_.clear(); page_.clear(); inFlight_=0; retryMs_=1000; jevRetryAt_=0; initial_=true;
    log_.close(); settings_.jevKey.clear(); settings_.youtubeKey.clear();
    oauth_={}; accessToken_.clear(); accessTokenUntil_=0; refreshPending_=false;
}
QNetworkReply *Engine::get(const QString &path,const QList<QPair<QString,QString>> &params) {
    QUrl url("https://www.googleapis.com/youtube/v3/"+path); QUrlQuery query;
    for (const auto &p:params) query.addQueryItem(p.first,p.second);
    url.setQuery(query);
    auto req=request(url);
    if (!oauth_.isEmpty()) req.setRawHeader("Authorization","Bearer "+accessToken_.toUtf8());
    else req.setRawHeader("X-Goog-Api-Key",settings_.youtubeKey.toUtf8());
    auto *reply=network_->get(req); replies_.insert(reply); return reply;
}
void Engine::youtubeFailure(QNetworkReply *reply) {
    const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status==401 || status==403) {
        emit youtubeStatus("YouTube: 認証・権限・クォータを確認して再接続");
        return;
    }
    if (status==404) { chatId_.clear(); page_.clear(); initial_=true; }
    emit youtubeStatus("YouTube disconnected — reconnecting...");
    poll_->start(retryMs_); retryMs_=std::min(retryMs_*2,30000);
}
void Engine::refreshYoutubeToken() {
    if (refreshPending_) return;
    refreshPending_=true;
    auto req=request(QUrl("https://oauth2.googleapis.com/token"));
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QByteArray form="grant_type=refresh_token";
    for (const auto field:{"client_id","client_secret","refresh_token"}) form += QByteArray("&")+field+"="+QUrl::toPercentEncoding(oauth_.value(field).toString());
    auto *reply=network_->post(req,form);
    replies_.insert(reply); const int generation=generation_;
    connect(reply,&QNetworkReply::finished,this,[this,reply,generation] {
        replies_.remove(reply); reply->deleteLater();
        if (!running_ || generation!=generation_) return;
        refreshPending_=false;
        const auto result=QJsonDocument::fromJson(reply->readAll()).object();
        const auto token=result.value("access_token").toString();
        const auto lifetime=result.value("expires_in").toInt();
        if (reply->error()!=QNetworkReply::NoError || token.isEmpty() || lifetime<=60) {
            emit youtubeStatus(QString("YouTube: OAuth認証失敗 (HTTP %1 / network %2)。再接続してください").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()).arg(int(reply->error()))); return;
        }
        accessToken_=token; accessTokenUntil_=nowMs()+qint64(lifetime-60)*1000;
        pollYoutube();
    });
}
void Engine::pollYoutube() {
    if (!running_) return;
    if (!oauth_.isEmpty() && nowMs()>=accessTokenUntil_) { refreshYoutubeToken(); return; }
    const bool resolve=chatId_.isEmpty();
    auto *reply= resolve ? get("videos",{{"part","liveStreamingDetails"},{"id",settings_.video}})
                        : get("liveChat/messages",{{"part","snippet,authorDetails"},{"liveChatId",chatId_},{"maxResults","200"},{"pageToken",page_}});
    const int generation=generation_;
    connect(reply,&QNetworkReply::finished,this,[this,reply,resolve,generation] {
        replies_.remove(reply); reply->deleteLater();
        if (!running_ || generation!=generation_) return;
        if (reply->error()!=QNetworkReply::NoError) { youtubeFailure(reply); return; }
        QJsonParseError error; auto doc=QJsonDocument::fromJson(reply->readAll(),&error);
        if (error.error!=QJsonParseError::NoError || !doc.isObject()) { youtubeFailure(reply); return; }
        const auto root=doc.object(); retryMs_=1000;
        if (resolve) {
            auto items=root.value("items").toArray();
            if (!items.isEmpty()) chatId_=items.first().toObject().value("liveStreamingDetails").toObject().value("activeLiveChatId").toString();
            if (chatId_.isEmpty()) { emit youtubeStatus("YouTube: ライブ開始／チャット有効化待ち"); poll_->start(15000); }
            else poll_->start(0);
            return;
        }
        if (!root.value("items").isArray() || !root.value("pollingIntervalMillis").isDouble() || root.value("nextPageToken").toString().isEmpty()) {
            emit youtubeStatus("YouTube: invalid response — reconnecting..."); poll_->start(5000); return;
        }
        const auto received=nowMs();
        for (const auto &c:chatComments(root,received)) {
            auto id=QString::fromStdString(c.id);
            if (seen_.contains(id)) continue;
            seen_.insert(id); seenOrder_.append(id);
            while (seenOrder_.size()>10000) seen_.remove(seenOrder_.takeFirst());
            // The first page is historical. Establish a cursor without recommending old chat.
            if (initial_) continue;
            writeEvent("comment_received",c);
            if (!filter_.accept(c)) { writeEvent("comment_filtered",c); continue; }
            if (pending_.size()>=128) { writeEvent("comment_dropped_overload",pending_.front()); pending_.pop_front(); }
            pending_.push_back(c);
        }
        initial_=false; page_=root.value("nextPageToken").toString();
        if (!root.value("offlineAt").toString().isEmpty()) { emit youtubeStatus("YouTube: 配信終了"); return; }
        emit youtubeStatus("● YouTube connected");
        poll_->start(std::max(1000,root.value("pollingIntervalMillis").toInt()));
    });
}
void Engine::flush() {
    if (!running_ || pending_.empty() || inFlight_>=2 || nowMs()<jevRetryAt_) return;
    std::vector<Comment> batch;
    while (!pending_.empty() && batch.size()<16) {
        auto c=pending_.front(); pending_.pop_front();
        if (nowMs()-c.received>10000) writeEvent("comment_dropped_stale",c); else batch.push_back(std::move(c));
    }
    if (batch.empty()) return;
    auto req=request(QUrl("https://api.typesafe.ai/v1/systemone"));
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    req.setRawHeader("Authorization","Bearer "+settings_.jevKey.toUtf8());
    auto *reply=network_->post(req,QJsonDocument(evaluationRequest(batch,settings_.evaluationPrompt)).toJson(QJsonDocument::Compact));
    replies_.insert(reply); ++inFlight_; const int generation=generation_;
    connect(reply,&QNetworkReply::finished,this,[this,reply,batch,generation] {
        replies_.remove(reply); reply->deleteLater();
        if (!running_ || generation!=generation_) return;
        --inFlight_;
        QJsonParseError error; const auto document=QJsonDocument::fromJson(reply->readAll(),&error);
        auto results=error.error==QJsonParseError::NoError && document.isObject() ? evaluationResponse(document.object(),batch) : std::nullopt;
        if (reply->error()!=QNetworkReply::NoError || !results) {
            emit jevStatus("AI Filter unavailable");
            bool ok=false; const auto seconds=reply->rawHeader("Retry-After").toInt(&ok);
            jevRetryAt_=nowMs()+(ok?std::clamp(seconds,1,300)*1000:5000);
            for (const auto &c:batch) writeEvent("comment_unscored",c);
            return;
        }
        emit jevStatus("● Jev connected");
        for (const auto &s:*results) {
            writeEvent("comment_scored",s.comment,&s,nowMs()-s.comment.received);
            if (s.hide) writeEvent("comment_hidden",s.comment,&s,nowMs()-s.comment.received);
        }
        emit scored(*results,settings_.session);
    });
}
void Engine::feedback(QString event,Scored comment,qint64 latency) {
    if (!running_ || !QStringList{"comment_shown","thumbs_up","dismissed"}.contains(event)) return;
    writeEvent(event,comment.comment,&comment,latency);
}
void Engine::writeEvent(const QString &event,const Comment &c,const Scored *s,Millis latency) {
    if (!log_.isOpen()) return;
    if (log_.size()>5*1024*1024) {
        const auto path=log_.fileName(); log_.close();
        QFile::remove(path+".2"); QFile::rename(path+".1",path+".2"); QFile::rename(path,path+".1");
        if (!log_.open(QIODevice::WriteOnly|QIODevice::Append)) { emit logStatus("ログを保存できません"); return; }
        log_.setPermissions(QFile::ReadOwner|QFile::WriteOwner);
    }
    QJsonObject record{{"timestamp",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"event",event},{"comment_id",QString::fromStdString(c.id)},
        {"hide",s?QJsonValue(s->hide?"hide":"show"):QJsonValue()},{"pick_value",s?QJsonValue(name(s->pick)):QJsonValue()},
        {"comment_type",s?QJsonValue(name(s->type)):QJsonValue()},{"latency_ms",latency>=0?QJsonValue(double(latency)):QJsonValue()}};
    if (event=="comment_received" && c.published>0) record.insert("youtube_delivery_ms",double(QDateTime::currentMSecsSinceEpoch()-c.published));
    const auto line=QJsonDocument(record).toJson(QJsonDocument::Compact)+'\n';
    if (log_.write(line)!=line.size() || !log_.flush()) emit logStatus("ログ書き込み失敗");
}
}
