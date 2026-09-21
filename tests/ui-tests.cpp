#include "ui/comment-dock.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
namespace comments {
struct DockTestAccess {
    static void run(CommentDock &dock) {
        dock.active_=true;
        const auto now=nowMs();
        Scored question{{"q","u","B2Cだと誰がAPI代払うの？",now},false,Pick::High,Type::Question};
        Scored hostile{{"bad","u","表示してはいけない攻撃コメント",now},true,Pick::High,Type::Other};
        Scored markup{{"html","u","<b>HTML should remain plain text</b>",now},false,Pick::Medium,Type::Opinion};
        dock.engine_->scored({question,hostile,markup},dock.session_);
        QApplication::processEvents();
        CHECK(dock.cards_.size()==2); CHECK(!dock.cards_.contains("bad"));
        bool plain=false;
        for (auto *label:dock.findChildren<QLabel *>()) if (label->text().contains("<b>")) plain=label->textFormat()==Qt::PlainText;
        CHECK(plain);
        int up=0,dismiss=0;
        QObject::connect(&dock,&CommentDock::feedback,&dock,[&](QString event,Scored,qint64){ if(event=="thumbs_up")++up; if(event=="dismissed")++dismiss; });
        auto *card=dock.cards_.value("q");
        for (auto *button:card->findChildren<QPushButton *>()) if(button->text()=="👍") { button->click(); button->click(); }
        CHECK(up==1);
        for (auto *button:card->findChildren<QPushButton *>()) if(button->text()=="×") button->click();
        CHECK(dismiss==1); CHECK(!dock.cards_.contains("q"));
        dock.engine_->scored({question},dock.session_-1); QApplication::processEvents(); CHECK(!dock.cards_.contains("q"));
        dock.shutdown(); CHECK(!dock.worker_.isRunning());
    }
};
}
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir dir;
    comments::CommentDock dock(dir.path()); dock.resize(400,700); dock.show();
    comments::DockTestAccess::run(dock);
    std::cout<<"UI: hostile excluded, plain text, one-shot thumbs, dismiss, stale-session rejection, worker shutdown PASS\n";
}
