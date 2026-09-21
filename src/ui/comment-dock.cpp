#include "comment-dock.hpp"
#include "youtube/youtube-chat.hpp"
#include <QFormLayout>
#include <QScrollArea>
#include <QSettings>
#include <QDir>
#include <QFrame>
namespace comments {
CommentDock::CommentDock(QString configDirectory,QWidget *parent):QWidget(parent),engine_(new Engine),configDirectory_(std::move(configDirectory)) {
    qRegisterMetaType<Settings>(); qRegisterMetaType<Scored>(); qRegisterMetaType<ScoredBatch>();
    setMinimumWidth(290);
    auto *layout=new QVBoxLayout(this);
    auto *title=new QLabel("AI Comments",this); auto font=title->font(); font.setBold(true); font.setPointSize(16); title->setFont(font); layout->addWidget(title);
    youtubeStatus_=new QLabel("YouTube: disconnected",this); jevStatus_=new QLabel("Jev: disconnected",this);
    youtubeStatus_->setWordWrap(true); jevStatus_->setWordWrap(true);
    layout->addWidget(youtubeStatus_); layout->addWidget(jevStatus_);
    auto *form=new QFormLayout;
    video_=new QLineEdit(this); video_->setPlaceholderText("YouTube Live URL / Video ID");
    youtubeKey_=new QLineEdit(this); youtubeKey_->setEchoMode(QLineEdit::Password); youtubeKey_->setPlaceholderText("YouTube Data API key");
    jevKey_=new QLineEdit(this); jevKey_->setEchoMode(QLineEdit::Password); jevKey_->setPlaceholderText("Jev API key");
    youtubeKey_->setText(qEnvironmentVariable("YOUTUBE_API_KEY")); jevKey_->setText(qEnvironmentVariable("TYPESAFE_API_KEY"));
    form->addRow("YouTube",video_); form->addRow("YouTube API",youtubeKey_); form->addRow("Jev API",jevKey_);
    hide_=new QCheckBox("Hide hostile comments",this); hide_->setChecked(true); form->addRow(hide_);
    minimum_=new QComboBox(this); minimum_->addItems({"Medium","High"}); form->addRow("Minimum recommendation",minimum_); layout->addLayout(form);
    start_=new QPushButton("接続",this); layout->addWidget(start_);
    auto *scroll=new QScrollArea(this); scroll->setWidgetResizable(true);
    auto *container=new QWidget(scroll); list_=new QVBoxLayout(container); list_->setAlignment(Qt::AlignTop);
    empty_=new QLabel("新着コメントの推薦をここに表示します",container); empty_->setWordWrap(true); list_->addWidget(empty_);
    scroll->setWidget(container); layout->addWidget(scroll,1);
    logStatus_=new QLabel(this); logStatus_->setWordWrap(true); layout->addWidget(logStatus_);
    QSettings saved(configDirectory_+"/settings.ini",QSettings::IniFormat);
    video_->setText(saved.value("video").toString());
    hide_->setChecked(saved.value("hide",true).toBool()); minimum_->setCurrentIndex(saved.value("minimum",0).toInt()==1?1:0);
    engine_->moveToThread(&worker_);
    connect(&worker_,&QThread::finished,engine_,&QObject::deleteLater);
    connect(this,&CommentDock::begin,engine_,&Engine::start);
    connect(this,&CommentDock::halt,engine_,&Engine::stop);
    connect(this,&CommentDock::feedback,engine_,&Engine::feedback);
    connect(engine_,&Engine::youtubeStatus,youtubeStatus_,&QLabel::setText);
    connect(engine_,&Engine::jevStatus,jevStatus_,&QLabel::setText);
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
        active_=false; emit halt(); start_->setText("接続");
        video_->setEnabled(true); youtubeKey_->setEnabled(true); jevKey_->setEnabled(true);
        buffer_=RecommendationBuffer{}; render();
        youtubeStatus_->setText("YouTube: disconnected"); jevStatus_->setText("Jev: disconnected"); return;
    }
    auto id=videoId(video_->text());
    if (id.isEmpty() || youtubeKey_->text().trimmed().isEmpty() || jevKey_->text().trimmed().isEmpty()) {
        logStatus_->setText("有効なLive URL / Video IDと、YouTube・JevのAPIキーを入力してください。"); return;
    }
    buffer_=RecommendationBuffer{}; shown_.clear(); shownOrder_.clear(); active_=true;
    QDir().mkpath(configDirectory_);
    QSettings saved(configDirectory_+"/settings.ini",QSettings::IniFormat);
    saved.setValue("video",video_->text()); saved.setValue("hide",hide_->isChecked()); saved.setValue("minimum",minimum_->currentIndex());
    // Credentials stay in memory and are never written to settings or logs.
    emit begin({id,youtubeKey_->text().trimmed(),jevKey_->text().trimmed(),configDirectory_,++session_});
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
    for (size_t i=0;i<items.size();++i) {
        const auto s=items[i]; const auto id=QString::fromStdString(s.comment.id);
        auto *card=cards_.value(id,nullptr);
        if (!card) {
            card=new QFrame; card->setObjectName("commentCard"); auto *column=new QVBoxLayout(card);
            auto *body=new QLabel(QString(s.pick==Pick::High?"★ ":"")+QString::fromStdString(s.comment.text),card);
            body->setTextFormat(Qt::PlainText); body->setWordWrap(true); body->setTextInteractionFlags(Qt::TextSelectableByMouse); column->addWidget(body);
            auto *row=new QHBoxLayout; auto *type=new QLabel(QString::fromUtf8(name(s.type)),card); row->addWidget(type); row->addStretch();
            auto *good=new QPushButton("👍",card); good->setAccessibleName("良い推薦だった"); good->setFixedWidth(40);
            auto *dismiss=new QPushButton("×",card); dismiss->setAccessibleName("このコメントは不要"); dismiss->setFixedWidth(32);
            row->addWidget(good); row->addWidget(dismiss); column->addLayout(row);
            connect(good,&QPushButton::clicked,this,[this,s,good]{ good->setEnabled(false); emit feedback("thumbs_up",s,nowMs()-s.comment.received); });
            connect(dismiss,&QPushButton::clicked,this,[this,s]{ emit feedback("dismissed",s,nowMs()-s.comment.received); buffer_.dismiss(s.comment.id); render(); });
            cards_.insert(id,card);
        }
        auto font=card->font(); font.setBold(i<4); card->setFont(font);
        card->setStyleSheet(i<4?"QFrame#commentCard { border-left: 3px solid #74b9ff; padding: 4px; }":"QFrame#commentCard { padding: 4px; }");
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
