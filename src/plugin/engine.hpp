#pragma once
#include "core/filter.hpp"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSet>
#include <QStringList>
#include <QFile>
#include <QJsonObject>
#include <deque>
#include <chrono>
namespace comments {
inline Millis nowMs() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct Settings { QString video, youtubeKey, jevKey, logDirectory; int session=0; QString evaluationPrompt; };
using ScoredBatch=std::vector<Scored>;
class Engine: public QObject {
    Q_OBJECT
    Settings settings_;
    QNetworkAccessManager *network_=nullptr;
    QTimer *poll_=nullptr, *batchTimer_=nullptr;
    QSet<QNetworkReply *> replies_;
    BasicFilter filter_;
    std::deque<Comment> pending_;
    QSet<QString> seen_;
    QStringList seenOrder_;
    QString chatId_, page_;
    QJsonObject oauth_;
    QString accessToken_;
    Millis accessTokenUntil_=0;
    bool refreshPending_=false;
    void refreshYoutubeToken();
    QFile log_;
    int generation_=0, inFlight_=0, retryMs_=1000;
    Millis jevRetryAt_=0;
    bool running_=false, initial_=true;
    QNetworkReply *get(const QString &path, const QList<QPair<QString,QString>> &params);
    void pollYoutube();
    void flush();
    void youtubeFailure(QNetworkReply *reply);
    void writeEvent(const QString &event, const Comment &c, const Scored *s=nullptr, Millis latency=-1);
public:
    using QObject::QObject;
public slots:
    void updateEvaluationPrompt(QString prompt);
    void restoreJevKey();
    void storeJevKey(QString key);
    void start(Settings settings);
    void stop();
    void feedback(QString event, Scored comment, qint64 latency);
signals:
    void jevKeyRestored(QString key);
    void credentialStatus(QString status);
    void scored(ScoredBatch batch, int session);
    void youtubeStatus(QString status);
    void jevStatus(QString status);
    void logStatus(QString status);
};
}
Q_DECLARE_METATYPE(comments::Settings)
Q_DECLARE_METATYPE(comments::Scored)
Q_DECLARE_METATYPE(comments::ScoredBatch)
