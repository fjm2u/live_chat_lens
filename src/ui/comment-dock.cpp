#include "comment-dock.hpp"
#include "youtube/youtube-chat.hpp"
#include <QFormLayout>
#include <QScrollArea>
#include <QSettings>
#include <QDir>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>
#include <QDialog>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include "jev/jev-client.hpp"
namespace comments {
CommentDock::CommentDock(QString configDirectory,QWidget *parent):QWidget(parent),engine_(new Engine),configDirectory_(std::move(configDirectory)) {
    qRegisterMetaType<Settings>(); qRegisterMetaType<Scored>(); qRegisterMetaType<ScoredBatch>();
    setMinimumWidth(290);
    setObjectName("aiComments");
    setStyleSheet(R"(
        QWidget#aiComments { background: #171b23; color: #edf1f7; }
        QWidget#aiComments QLabel { background: transparent; color: #edf1f7; border: none; }
        QWidget#aiComments QLabel#muted { color: #a5b1c3; font-size: 11px; }
        QWidget#aiComments QLabel#status { color: #a5b1c3; font-size: 11px; }
        QWidget#aiComments QFrame#commentCard { background: #242b37; border: 1px solid #354052; border-radius: 12px; }
        QWidget#aiComments QLabel#commentBody { font-size: 16px; }
        QWidget#aiComments QPushButton { background: #2c3545; color: #edf1f7; border: 1px solid #43516a; border-radius: 7px; padding: 6px 10px; }
        QWidget#aiComments QPushButton:hover { background: #3b4960; }
        QWidget#aiComments QPushButton:focus { border: 1px solid #99bdff; }
        QWidget#aiComments QPushButton:disabled { color: #8a99ae; background: #252d39; }
        QWidget#aiComments QPushButton#primary { background: #8eb8ff; color: #142039; font-weight: 600; }
        QWidget#aiComments QLineEdit, QWidget#aiComments QComboBox { background: #222a36; color: #edf1f7; border: 1px solid #44536b; border-radius: 6px; padding: 7px; }
        QWidget#aiComments QCheckBox { color: #edf1f7; }
        QWidget#aiComments QScrollArea { background: transparent; border: none; }
        QWidget#commentList { background: #171b23; }
    )");
    auto *layout=new QVBoxLayout(this); layout->setContentsMargins(14,16,14,12); layout->setSpacing(12);
    auto *header=new QHBoxLayout;
    auto *title=new QLabel("AI Comments",this); auto font=title->font(); font.setBold(true); font.setPointSize(17); title->setFont(font); header->addWidget(title); header->addStretch();
    settingsToggle_=new QPushButton("設定",this); settingsToggle_->setCheckable(true); settingsToggle_->setChecked(true); settingsToggle_->setAccessibleName("接続設定を開閉"); header->addWidget(settingsToggle_); layout->addLayout(header);
    youtubeStatus_=new QLabel("○ YouTube 未接続",this); jevStatus_=new QLabel("○ Jev 未接続",this);
    auto *statusRow=new QHBoxLayout;
    for (auto *label:{youtubeStatus_,jevStatus_}) { label->setObjectName("status"); label->setWordWrap(true); label->setTextFormat(Qt::PlainText); statusRow->addWidget(label,1); }
    layout->addLayout(statusRow);
    settingsPanel_=new QWidget(this); auto *settingsLayout=new QVBoxLayout(settingsPanel_); settingsLayout->setContentsMargins(0,4,0,4); settingsLayout->setSpacing(10);
    connect(settingsToggle_,&QPushButton::toggled,settingsPanel_,&QWidget::setVisible);
    QSettings saved(configDirectory_+"/settings.ini",QSettings::IniFormat);
    auto *form=new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    video_=new QLineEdit(this); video_->setPlaceholderText("YouTube Live URL / Video ID");
    youtubeKey_=new QLineEdit(this); youtubeKey_->setEchoMode(QLineEdit::Password); youtubeKey_->setPlaceholderText("YouTube Data API key");
    jevKey_=new QLineEdit(this); jevKey_->setEchoMode(QLineEdit::Password); jevKey_->setPlaceholderText("Jev API key");
    youtubeKey_->setText(qEnvironmentVariable("YOUTUBE_API_KEY")); jevKey_->setText(qEnvironmentVariable("TYPESAFE_API_KEY"));
    // Reuse only an explicitly configured location or the known local credential
    // used by this installation. Never scan other apps' credential stores.
    if (youtubeKey_->text().isEmpty()) {
        QString oauthPath=qEnvironmentVariable("YOUTUBE_OAUTH_FILE");
        if (oauthPath.isEmpty()) oauthPath=saved.value("youtubeOAuthFile").toString();
        if (oauthPath.isEmpty()) oauthPath=QDir::homePath()+"/.config/english-video-growth/youtube-token.json";
        const QFileInfo file(oauthPath);
        if (file.isAbsolute() && file.isFile() && file.isReadable())
            youtubeKey_->setText("oauth:"+file.absoluteFilePath());
    }
    auto *authRow=new QWidget(this); auto *authRowLayout=new QHBoxLayout(authRow); authRowLayout->setContentsMargins(0,0,0,0);
    auto *authSummary=new QLabel(this); authSummary->setObjectName("muted"); authSummary->setWordWrap(true);
    auto *authChange=new QPushButton("変更",this); authChange->setCheckable(true); authChange->setAccessibleName("YouTube認証の手動設定を開閉");
    authRowLayout->addWidget(authSummary,1); authRowLayout->addWidget(authChange);
    auto *authDetails=new QWidget(this); authDetails->setObjectName("youtubeAuthDetails");
    auto *authForm=new QVBoxLayout(authDetails); authForm->setContentsMargins(0,0,0,0);
    authForm->addWidget(youtubeKey_);
    auto *oauthButton=new QPushButton("認証ファイルを選ぶ",this); authForm->addWidget(oauthButton);
    connect(authChange,&QPushButton::toggled,authDetails,&QWidget::setVisible);
    auto updateAuthSummary=[this,authSummary]{
        authSummary->setText(youtubeKey_->text().isEmpty()?"YouTube認証を設定してください":
            youtubeKey_->text().startsWith("oauth:")?"YouTube認証を設定済み":"YouTube APIキーを設定済み");
    };
    connect(youtubeKey_,&QLineEdit::textChanged,this,updateAuthSummary);
    updateAuthSummary();
    authChange->setChecked(youtubeKey_->text().isEmpty()); authDetails->setVisible(authChange->isChecked());
    connect(oauthButton,&QPushButton::clicked,this,[this,authChange]{
        if (active_) return;
        const auto file=QFileDialog::getOpenFileName(this,"YouTube認証ファイル",QDir::homePath(),"JSON (*.json)");
        if (!file.isEmpty()) {
            youtubeKey_->setText("oauth:"+file);
            QSettings saved(configDirectory_+"/settings.ini",QSettings::IniFormat);
            saved.setValue("youtubeOAuthFile",file); // Path only; never copy credentials.
            authChange->setChecked(false);
        }
    });
    form->addRow("YouTube",video_); form->addRow(authRow); form->addRow(authDetails); form->addRow("Jev API",jevKey_);
    hide_=new QCheckBox("攻撃的なコメントを非表示",this); hide_->setChecked(true); form->addRow(hide_);
    minimum_=new QComboBox(this); minimum_->addItems({"標準以上","高評価のみ"}); form->addRow("表示するコメント",minimum_); settingsLayout->addLayout(form);
    auto *promptButton=new QPushButton("AIの判定方針を編集",this); settingsLayout->addWidget(promptButton);
    connect(promptButton,&QPushButton::clicked,this,&CommentDock::editPrompt);
    start_=new QPushButton("接続",this); start_->setObjectName("primary"); settingsLayout->addWidget(start_); layout->addWidget(settingsPanel_);
    auto *scroll=new QScrollArea(this); scroll->setWidgetResizable(true);
    auto *container=new QWidget(scroll); container->setObjectName("commentList"); list_=new QVBoxLayout(container); list_->setContentsMargins(0,0,4,0); list_->setSpacing(10); list_->setAlignment(Qt::AlignTop); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    empty_=new QLabel("拾いたいコメントを、ここに。\n\n接続すると、おすすめの新着コメントが届きます。",container); empty_->setObjectName("muted"); empty_->setAlignment(Qt::AlignCenter); empty_->setContentsMargins(16,40,16,40); empty_->setWordWrap(true); list_->addWidget(empty_);
    scroll->setWidget(container); layout->addWidget(scroll,1);
    logStatus_=new QLabel(this); logStatus_->setWordWrap(true); layout->addWidget(logStatus_);
    evaluationPrompt_=saved.value("jevPrompt",defaultEvaluationPrompt()).toString();
    video_->setText(saved.value("video").toString());
    hide_->setChecked(saved.value("hide",true).toBool()); minimum_->setCurrentIndex(saved.value("minimum",0).toInt()==1?1:0);
    auto *credentialStatus=new QLabel(this); credentialStatus->setWordWrap(true); credentialStatus->setTextFormat(Qt::PlainText); layout->addWidget(credentialStatus);
    connect(engine_,&Engine::credentialStatus,credentialStatus,&QLabel::setText);
    connect(engine_,&Engine::jevKeyRestored,this,[this](const QString &key){
        if (jevKey_->text().isEmpty() && !jevKey_->isModified()) jevKey_->setText(key);
    });
    connect(this,&CommentDock::saveCredential,engine_,&Engine::storeJevKey);
    connect(jevKey_,&QLineEdit::editingFinished,this,[this]{
        if (!jevKey_->text().trimmed().isEmpty()) emit saveCredential(jevKey_->text());
    });
    engine_->moveToThread(&worker_);
    connect(&worker_,&QThread::started,engine_,&Engine::restoreJevKey);
    connect(&worker_,&QThread::finished,engine_,&QObject::deleteLater);
    connect(this,&CommentDock::begin,engine_,&Engine::start);
    connect(this,&CommentDock::promptChanged,engine_,&Engine::updateEvaluationPrompt);
    connect(this,&CommentDock::halt,engine_,&Engine::stop);
    connect(this,&CommentDock::feedback,engine_,&Engine::feedback);
    connect(engine_,&Engine::youtubeStatus,this,[this](const QString &s){ updateConnection(true,s); });
    connect(engine_,&Engine::jevStatus,this,[this](const QString &s){ updateConnection(false,s); });
    connect(engine_,&Engine::logStatus,logStatus_,&QLabel::setText);
    connect(engine_,&Engine::scored,this,[this](const ScoredBatch &batch,int session){
        if (!active_ || session!=session_) return;
        for (const auto &s:batch) buffer_.add(s);
        render();
    });
    connect(start_,&QPushButton::clicked,this,&CommentDock::toggle);
    connect(hide_,&QCheckBox::toggled,this,[this]{ render(); });
    connect(minimum_,&QComboBox::currentIndexChanged,this,[this]{ render(); });
    connect(&refresh_,&QTimer::timeout,this,&CommentDock::render); refresh_.start(100);
    worker_.start();
}
void CommentDock::editPrompt() {
    QDialog dialog(this); dialog.setWindowTitle("AIの判定方針"); dialog.resize(560,500);
    auto *layout=new QVBoxLayout(&dialog);
    auto *help=new QLabel("非表示にする内容や、優先したい質問・話題を指定できます。\n保存後の新しい評価から反映されます。評価中・表示済みのコメントは変更しません。",&dialog);
    help->setWordWrap(true); layout->addWidget(help);
    auto *editor=new QPlainTextEdit(evaluationPrompt_,&dialog); editor->setAccessibleName("Jev判定プロンプト"); layout->addWidget(editor,1);
    auto *status=new QLabel(&dialog); layout->addWidget(status);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel|QDialogButtonBox::RestoreDefaults,&dialog);
    buttons->button(QDialogButtonBox::Save)->setText("保存");
    buttons->button(QDialogButtonBox::Cancel)->setText("キャンセル");
    buttons->button(QDialogButtonBox::RestoreDefaults)->setText("初期値に戻す");
    layout->addWidget(buttons);
    auto validate=[editor,status,buttons]{
        const auto size=editor->toPlainText().trimmed().size();
        const bool valid=size>0 && size<=6000;
        status->setText(QString("%1 / 6000文字").arg(size)+(valid?"":"  1〜6000文字で入力してください"));
        buttons->button(QDialogButtonBox::Save)->setEnabled(valid);
    };
    connect(editor,&QPlainTextEdit::textChanged,&dialog,validate); validate();
    connect(buttons->button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,&dialog,[editor]{editor->setPlainText(defaultEvaluationPrompt());});
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted) return;
    QSettings saved(configDirectory_+"/settings.ini",QSettings::IniFormat);
    const auto prompt=editor->toPlainText().trimmed(); saved.setValue("jevPrompt",prompt); saved.sync();
    if(saved.status()!=QSettings::NoError) { logStatus_->setText("判定方針を保存できませんでした"); return; }
    evaluationPrompt_=prompt; emit promptChanged(prompt);
}
void CommentDock::updateConnection(bool youtube,const QString &status) {
    if (!active_) return;
    const bool ready=status==(youtube?"● YouTube connected":"● Jev connected");
    (youtube?youtubeReady_:jevReady_)=ready;
    auto *label=youtube?youtubeStatus_:jevStatus_;
    label->setText(ready?(youtube?"● YouTube 接続済み":"● Jev 接続済み"):status);
    label->setStyleSheet(ready?"color: #8eddb9;":"color: #eac58a;");
    if (youtubeReady_ && jevReady_ && !collapsedOnce_) {
        collapsedOnce_=true; settingsToggle_->setChecked(false);
    }
}
CommentDock::~CommentDock() { shutdown(); }
void CommentDock::shutdown() {
    refresh_.stop(); active_=false;
    if (worker_.isRunning()) {
        QMetaObject::invokeMethod(engine_,&Engine::stop,Qt::BlockingQueuedConnection);
        worker_.quit(); worker_.wait();
    }
}
void CommentDock::toggle() {
    if (active_) {
        active_=false; emit halt(); start_->setText("接続"); settingsToggle_->setChecked(true);
        video_->setEnabled(true); youtubeKey_->setEnabled(true); jevKey_->setEnabled(true);
        buffer_=RecommendationBuffer{}; render();
        youtubeStatus_->setText("YouTube: disconnected"); jevStatus_->setText("Jev: disconnected"); return;
    }
    auto id=videoId(video_->text());
    if (id.isEmpty() || youtubeKey_->text().trimmed().isEmpty() || jevKey_->text().trimmed().isEmpty()) {
        logStatus_->setText("Live URL、YouTube認証、Jev APIキーを設定してください。"); return;
    }
    buffer_=RecommendationBuffer{}; shown_.clear(); shownOrder_.clear(); active_=true; youtubeReady_=jevReady_=collapsedOnce_=false;
    QDir().mkpath(configDirectory_);
    QSettings saved(configDirectory_+"/settings.ini",QSettings::IniFormat);
    saved.setValue("video",video_->text()); saved.setValue("hide",hide_->isChecked()); saved.setValue("minimum",minimum_->currentIndex());
    if (youtubeKey_->text().startsWith("oauth:")) saved.setValue("youtubeOAuthFile",youtubeKey_->text().mid(6));
    else saved.remove("youtubeOAuthFile");
    // Persist Jev only through the OS credential store, never INI or logs.
    emit saveCredential(jevKey_->text());
    emit begin({id,youtubeKey_->text().trimmed(),jevKey_->text().trimmed(),configDirectory_,++session_,evaluationPrompt_});
    start_->setText("切断"); video_->setEnabled(false); youtubeKey_->setEnabled(false); jevKey_->setEnabled(false);
    youtubeStatus_->setText("YouTube: connecting..."); jevStatus_->setText("Jev: waiting"); logStatus_->clear();
}
void CommentDock::render() {
    auto items=buffer_.update(nowMs(),minimum_->currentIndex()==1?Pick::High:Pick::Medium,hide_->isChecked());
    QSet<QString> keep;
    for (const auto &s:items) keep.insert(QString::fromStdString(s.comment.id));
    for (auto it=cards_.begin();it!=cards_.end();) {
        if (!keep.contains(it.key())) { auto *card=it.value(); list_->removeWidget(card); card->deleteLater(); it=cards_.erase(it); } else ++it;
    }
    empty_->setVisible(items.empty());
    empty_->setText(active_?"コメントを待っています\n\n拾いやすい質問や意見が届くと、ここに表示します。":"拾いたいコメントを、ここに。\n\n接続すると、おすすめの新着コメントが届きます。");
    for (size_t i=0;i<items.size();++i) {
        const auto s=items[i]; const auto id=QString::fromStdString(s.comment.id);
        auto *card=cards_.value(id,nullptr);
        if (!card) {
            card=new QFrame; card->setObjectName("commentCard"); auto *column=new QVBoxLayout(card); column->setContentsMargins(14,14,14,10); column->setSpacing(10);
            auto *body=new QLabel(QString(s.pick==Pick::High?"★ ":"")+QString::fromStdString(s.comment.text),card);
            body->setObjectName("commentBody"); body->setTextFormat(Qt::PlainText); body->setWordWrap(true); body->setTextInteractionFlags(Qt::TextSelectableByMouse); column->addWidget(body);
            auto *row=new QHBoxLayout;
            QString category;
            switch(s.type) { case Type::Question: category="質問"; break; case Type::Reaction: category="リアクション"; break; case Type::Joke: category="ユーモア"; break; case Type::Opinion: category="意見"; break; default: category="コメント"; }
            auto *type=new QLabel((s.pick==Pick::High?"おすすめ  ·  ":"")+category,card); type->setObjectName("muted"); row->addWidget(type); row->addStretch();
            auto *good=new QPushButton("👍",card); good->setAccessibleName("良い推薦だった"); good->setFixedSize(40,30); good->setToolTip("良い推薦だった");
            auto *dismiss=new QPushButton("×",card); dismiss->setAccessibleName("このコメントは不要"); dismiss->setFixedSize(32,30); dismiss->setToolTip("このコメントは不要");
            row->addWidget(good); row->addWidget(dismiss); column->addLayout(row);
            connect(good,&QPushButton::clicked,this,[this,s,good]{ good->setEnabled(false); emit feedback("thumbs_up",s,nowMs()-s.comment.received); });
            connect(dismiss,&QPushButton::clicked,this,[this,s]{ emit feedback("dismissed",s,nowMs()-s.comment.received); buffer_.dismiss(s.comment.id); render(); });
            cards_.insert(id,card);
        }
        auto *body=card->findChild<QLabel *>("commentBody"); auto font=body->font(); font.setBold(i<4); body->setFont(font);
        card->setStyleSheet(s.pick==Pick::High?"QFrame#commentCard { background: #28374c; border: 1px solid #739bd2; border-left: 3px solid #9bc3ff; }":"");
        // Move existing widgets only when the recommendation buffer changes order.
        if (list_->indexOf(card)!=int(i)) list_->insertWidget(int(i),card);
        if (isVisible() && !shown_.contains(id)) {
            shown_.insert(id); shownOrder_.append(id);
            while (shownOrder_.size()>10000) shown_.remove(shownOrder_.takeFirst());
            emit feedback("comment_shown",s,nowMs()-s.comment.received);
        }
    }
}
}
