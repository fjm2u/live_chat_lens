#include "jev/jev-client.hpp"
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QTimer>
#include <iostream>
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    std::string key; std::getline(std::cin,key);
    if (key.empty()) return 2;
    std::vector<comments::Comment> batch{{"synthetic-question","synthetic","B2Cだと誰がAPI代払うの？",0},
        {"synthetic-criticism","synthetic","そのビジネスモデル成立しなくない？",0},
        {"synthetic-hostile","synthetic","お前頭悪すぎ",0},
        {"synthetic-greeting","synthetic","こんにちは",0}};
    QNetworkAccessManager network; QNetworkRequest req(QUrl("https://api.typesafe.ai/v1/systemone"));
    req.setTransferTimeout(15000); req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json"); req.setRawHeader("Authorization",QByteArray("Bearer ")+QByteArray::fromStdString(key));
    key.assign(key.size(),'\0');
    QElapsedTimer elapsed; elapsed.start();
    auto *reply=network.post(req,QJsonDocument(comments::evaluationRequest(batch)).toJson(QJsonDocument::Compact));
    QObject::connect(reply,&QNetworkReply::finished,&app,[&] {
        std::cout<<"HTTP "<<reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()<<" elapsed_ms="<<elapsed.elapsed()<<"\n";
        auto parsed=comments::evaluationResponse(QJsonDocument::fromJson(reply->readAll()).object(),batch);
        if (reply->error()!=QNetworkReply::NoError || !parsed) { app.exit(1); return; }
        for (const auto &s:*parsed) std::cout<<s.comment.id<<" "<<(s.hide?"hide":"show")<<" "<<comments::name(s.pick)<<" "<<comments::name(s.type)<<"\n";
        app.exit(0);
    });
    QTimer::singleShot(20000,&app,[&]{app.exit(3);});
    return app.exec();
}
