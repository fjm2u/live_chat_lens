#pragma once
#include "core/comment.hpp"
#include <QJsonObject>
#include <QString>
#include <optional>
namespace comments {
QJsonObject evaluationRequest(const std::vector<Comment> &batch);
std::optional<std::vector<Scored>> evaluationResponse(const QJsonObject &root, const std::vector<Comment> &batch);
}
