#pragma once
#include "core/comment.hpp"
#include <QJsonObject>
#include <QString>
namespace comments {
QString videoId(const QString &input);
std::vector<Comment> chatComments(const QJsonObject &root, Millis received);
}
