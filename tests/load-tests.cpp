#include "ui/comment-dock.hpp"
#include "core/filter.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <algorithm>
#include <stdexcept>
#include <iostream>
using namespace comments;
#define REQUIRE(x) do { if(!(x)) throw std::runtime_error(#x); } while(false)
namespace comments {
struct DockTestAccess {
static QJsonObject run(CommentDock &dock,int count) {
    BasicFilter filter; RecommendationBuffer buffer;
    dock.active_=true;
    std::vector<double> times; int accepted=0, rejected=0, hidden=0, maxCards=0;
    QElapsedTimer total; total.start();
    ScoredBatch pending;
    const auto now=nowMs();
    auto flush=[&]{
        QElapsedTimer timer; timer.start();
        dock.engine_->scored(pending,dock.session_);
        QApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
        REQUIRE(dock.cards_.size()<=10);
        for(auto it=dock.cards_.begin();it!=dock.cards_.end();++it) {
            REQUIRE(!it.key().startsWith("hostile-"));
            REQUIRE(!it.key().startsWith("low-"));
        }
        maxCards=std::max(maxCards,int(dock.cards_.size()));
        times.push_back(timer.nsecsElapsed()/1000000.0); pending.clear();
    };
    for(int i=0;i<count;++i) {
        const int kind=i%10;
        const std::string prefix=kind==4?"hostile-":kind==5?"low-":"normal-";
        Comment c{prefix+std::to_string(i),"viewer-"+std::to_string(i),"質問 "+std::to_string(i),now};
        if(kind==0) c.text="";
        if(kind==1) c.text="https://example.com";
        if(kind==2) c.text=std::string(3000,'x');
        if(kind==3) c.text="完全重複";
        if(!filter.accept(c)) {++rejected;continue;}
        ++accepted;
        Scored s{c,kind==4,kind==5?Pick::Low:kind==6?Pick::High:Pick::Medium,Type(i%5)};
        if(s.hide)++hidden;
        // These labels are synthetic fixtures, not AI outputs.
        buffer.add(s); pending.push_back(s);
        if(pending.size()==16) flush();
    }
    if(!pending.empty())flush();
    REQUIRE(accepted+rejected==count); REQUIRE(rejected>0); REQUIRE(hidden>0);
    // Same-author flood, isolated from the mixed workload.
    BasicFilter burst; int burstAccepted=0;
    for(int i=0;i<1000;++i) if(burst.accept({std::to_string(i),"one-user","連投 "+std::to_string(i),now}))++burstAccepted;
    REQUIRE(burstAccepted==3);
    // Stable positions for medium cards, then expiry and recovery.
    RecommendationBuffer stable;
    for(int i=0;i<10;++i)stable.add({{"stable-"+std::to_string(i),"u","質問",now},false,Pick::Medium,Type::Question});
    auto first=stable.update(now);
    stable.add({{"new","u","新しい質問",now+1000},false,Pick::Medium,Type::Question});
    auto second=stable.update(now+1000); REQUIRE(first.size()==second.size());
    for(size_t i=0;i<first.size();++i)REQUIRE(first[i].comment.id==second[i].comment.id);
    REQUIRE(buffer.update(now+60001).empty());
    buffer.add({{"recovery","u","復旧",now+60002},false,Pick::High,Type::Question});
    REQUIRE(buffer.update(now+60002).size()==1);
    std::sort(times.begin(),times.end());
    auto p95=times.at((times.size()*95+99)/100-1);
    auto elapsed=total.elapsed();
    dock.shutdown(); REQUIRE(!dock.worker_.isRunning());
    return {{"mode","local_synthetic_scores_no_network"},{"input",count},{"accepted",accepted},{"filtered",rejected},
        {"hostile_fixture_count",hidden},{"max_visible_cards",maxCards},{"same_author_accepted_of_1000",burstAccepted},
        {"ui_batches",int(times.size())},{"ui_batch_p95_ms",p95},{"ui_batch_max_ms",times.back()},
        {"total_ms",elapsed},{"stability_expiry_recovery","PASS"},{"result","PASS"}};
}
};
}
int main(int argc,char **argv) {
 QApplication app(argc,argv); QTemporaryDir dir;
 CommentDock dock(dir.path()); dock.resize(360,720); dock.show();
 auto report=DockTestAccess::run(dock,10000);
 std::cout<<QJsonDocument(report).toJson().constData();
}
