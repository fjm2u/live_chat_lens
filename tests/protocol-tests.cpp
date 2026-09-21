#include "jev/jev-client.hpp"
#include "youtube/youtube-chat.hpp"
#include <QJsonArray>
#include <iostream>
#include <stdexcept>
using namespace comments;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
int main() {
    CHECK(videoId("https://www.youtube.com/watch?v=abcdefghijk&x=1")=="abcdefghijk");
    CHECK(videoId("https://youtu.be/abcdefghijk")=="abcdefghijk");
    CHECK(videoId("https://youtube.com/live/abcdefghijk")=="abcdefghijk");
    CHECK(videoId("https://youtube.com.evil.test/watch?v=abcdefghijk").isEmpty());
    CHECK(videoId("../../bad").isEmpty());
    std::vector<Comment> batch{{"id-a","user-a","B2Cだと誰がAPI代払うの？",1000},{"id-b","user-b","お前頭悪すぎ",1001}};
    auto req=evaluationRequest(batch); CHECK(req["questions"].toObject().size()==6);
    CHECK(req["state"].toObject()["comments"].toArray().size()==2);
    CHECK(req["questions"].toObject()["1_hide"].toObject()["instructions"].toString().contains("comments[1].text"));
    CHECK(!evaluationResponse({},batch));
    QJsonObject answers;
    for (int i=0;i<2;++i) for (auto field:{"hide","pick","type"}) {
        QString value=QString(field)=="hide"?(i?"hide":"show"):QString(field)=="pick"?"high":"question";
        answers.insert(QString::number(i)+"_"+field,QJsonObject{{"type","choice"},{"choice",value}});
    }
    auto valid=evaluationResponse({{"answers",answers}},batch); CHECK(valid); CHECK(valid->size()==2); CHECK(!(*valid)[0].hide); CHECK((*valid)[1].hide);
    answers.remove("1_hide"); CHECK(!evaluationResponse({{"answers",answers}},batch));
    answers.insert("1_hide",QJsonObject{{"type","choice"},{"choice","allow"}}); CHECK(!evaluationResponse({{"answers",answers}},batch));
    QJsonObject event{{"id","a"},{"authorDetails",QJsonObject{{"channelId","user"}}},{"snippet",QJsonObject{{"type","textMessageEvent"},{"displayMessage","  hello  "},{"publishedAt","2026-09-21T00:00:00.000Z"}}}};
    auto messages=chatComments({{"items",QJsonArray{event}}},1200); CHECK(messages.size()==1); CHECK(messages[0].text=="hello"); CHECK(messages[0].received==1200); CHECK(messages[0].published>0);
    std::cout<<"protocol: URL validation, batch mapping, fail-closed parsing, YouTube text PASS\n";
}
