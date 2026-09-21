#pragma once
#include "core/comment.hpp"
#include <QJsonObject>
#include <QString>
#include <optional>
namespace comments {
QString defaultEvaluationPrompt();
QJsonObject evaluationRequest(const std::vector<Comment> &batch, const QString &prompt = {});
std::optional<std::vector<Scored>> evaluationResponse(const QJsonObject &root, const std::vector<Comment> &batch);
}
