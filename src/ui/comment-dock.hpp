#pragma once
#include "plugin/engine.hpp"
#include "core/recommendation-buffer.hpp"
#include <QWidget>
#include <QThread>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QHash>
namespace comments {
class CommentDock: public QWidget {
    Q_OBJECT
    friend struct DockTestAccess;
    QThread worker_;
    Engine *engine_;
    QLineEdit *video_, *youtubeKey_, *jevKey_;
    QComboBox *minimum_;
    QCheckBox *hide_;
    QPushButton *start_, *settingsToggle_;
    QWidget *settingsPanel_;
    bool youtubeReady_=false, jevReady_=false, collapsedOnce_=false;
    void updateConnection(bool youtube, const QString &status);
    QLabel *youtubeStatus_, *jevStatus_, *logStatus_, *empty_;
    QVBoxLayout *list_;
    QTimer refresh_;
    RecommendationBuffer buffer_;
    QHash<QString,QWidget *> cards_;
    QSet<QString> shown_;
    QStringList shownOrder_;
    QString configDirectory_, evaluationPrompt_;
    void editPrompt();
    bool active_=false;
    int session_=0;
    void toggle();
    void render();
public:
    explicit CommentDock(QString configDirectory, QWidget *parent=nullptr);
    ~CommentDock() override;
    void shutdown();
signals:
    void promptChanged(QString prompt);
    void saveCredential(QString key);
    void begin(Settings settings);
    void halt();
    void feedback(QString event, Scored comment, qint64 latency);
};
}
