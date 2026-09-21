#include "ui/comment-dock.hpp"
#include "plugin/credential-store.hpp"
#include <QElapsedTimer>
#include <QDialog>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include "jev/jev-client.hpp"
#include <QApplication>
#include <QTemporaryDir>
#include <QTimer>
#include <QFile>
#include <QSettings>
#include <iostream>
#include <stdexcept>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
namespace comments {
struct DockTestAccess {
    static void credentialRestore(const QString &directory) {
        CHECK(saveJevKey("synthetic-test-key").isEmpty());
        qunsetenv("TYPESAFE_API_KEY");
        CommentDock dock(directory);
        QElapsedTimer wait; wait.start();
        while(dock.jevKey_->text().isEmpty() && wait.elapsed()<2000) QApplication::processEvents();
        CHECK(dock.jevKey_->text()=="synthetic-test-key");
        dock.jevKey_->setText("synthetic-updated-key");
        dock.jevKey_->editingFinished();
        // Flush queued writes before checking the test credential backend.
        QMetaObject::invokeMethod(dock.engine_,[]{},Qt::BlockingQueuedConnection);
        CHECK(loadJevKey().key=="synthetic-updated-key");
        QFile settings(directory+"/settings.ini");
        if(settings.open(QIODevice::ReadOnly)) CHECK(!settings.readAll().contains("synthetic-"));
    }
    static void authDefaults(const QString &directory) {
        qunsetenv("YOUTUBE_API_KEY");
        const auto path=directory+"/oauth.json";
        QFile fixture(path); CHECK(fixture.open(QIODevice::WriteOnly)); fixture.write("{}"); fixture.close();
        // No network request is made until Connect; engine validates the JSON.
        qputenv("YOUTUBE_OAUTH_FILE",path.toUtf8());
        { CommentDock dock(directory);
          CHECK(dock.youtubeKey_->text()=="oauth:"+path);
          CHECK(dock.findChild<QWidget *>("youtubeAuthDetails")->isHidden()); }
        qputenv("YOUTUBE_OAUTH_FILE",(directory+"/missing.json").toUtf8());
        { CommentDock dock(directory);
          CHECK(dock.youtubeKey_->text().isEmpty());
          CHECK(!dock.findChild<QWidget *>("youtubeAuthDetails")->isHidden()); }
        qunsetenv("YOUTUBE_OAUTH_FILE");
        { QSettings saved(directory+"/settings.ini",QSettings::IniFormat); saved.setValue("youtubeOAuthFile",path); }
        { CommentDock dock(directory); CHECK(dock.youtubeKey_->text()=="oauth:"+path); }
    }
    static void promptEditor(CommentDock &dock) {
        QTimer::singleShot(0,[&]{
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); CHECK(dialog);
            auto *editor=dialog->findChild<QPlainTextEdit *>();
            auto *buttons=dialog->findChild<QDialogButtonBox *>();
            editor->setPlainText(" "); CHECK(!buttons->button(QDialogButtonBox::Save)->isEnabled());
            editor->setPlainText(QString(6001,'x')); CHECK(!buttons->button(QDialogButtonBox::Save)->isEnabled());
            buttons->button(QDialogButtonBox::RestoreDefaults)->click(); CHECK(editor->toPlainText()==defaultEvaluationPrompt());
            editor->setPlainText("猫の質問を優先してください"); buttons->button(QDialogButtonBox::Save)->click();
        });
        dock.editPrompt(); CHECK(dock.evaluationPrompt_=="猫の質問を優先してください");
        QSettings saved(dock.configDirectory_+"/settings.ini",QSettings::IniFormat);
        CHECK(saved.value("jevPrompt").toString()==dock.evaluationPrompt_);
        { CommentDock restored(dock.configDirectory_); CHECK(restored.evaluationPrompt_==dock.evaluationPrompt_); }
        QTimer::singleShot(0,[]{
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            dialog->findChild<QPlainTextEdit *>()->setPlainText("discard"); dialog->reject();
        });
        dock.editPrompt(); CHECK(dock.evaluationPrompt_=="猫の質問を優先してください");
    }
    static void run(CommentDock &dock) {
        promptEditor(dock);
        dock.active_=true;
        dock.updateConnection(true,"● YouTube connected");
        CHECK(!dock.settingsPanel_->isHidden());
        dock.updateConnection(false,"● Jev connected");
        CHECK(dock.settingsPanel_->isHidden());
        dock.settingsToggle_->click(); CHECK(!dock.settingsPanel_->isHidden());
        dock.updateConnection(false,"● Jev connected"); CHECK(!dock.settingsPanel_->isHidden());
        dock.settingsToggle_->click();
        dock.updateConnection(false,"AI Filter unavailable");
        CHECK(dock.jevStatus_->text()=="AI Filter unavailable");
        dock.updateConnection(false,"● Jev connected");
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
        if (!qEnvironmentVariable("DOCK_PREVIEW_PATH").isEmpty()) {
            QApplication::processEvents(); dock.grab().save(qEnvironmentVariable("DOCK_PREVIEW_PATH"));
        }
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
    comments::DockTestAccess::authDefaults(dir.path());
    comments::DockTestAccess::credentialRestore(dir.path());
    comments::CommentDock dock(dir.path()); dock.resize(400,700); dock.show();
    comments::DockTestAccess::run(dock);
    std::cout<<"UI: hostile excluded, plain text, one-shot thumbs, dismiss, stale-session rejection, worker shutdown PASS\n";
}
