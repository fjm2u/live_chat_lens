#pragma once
#include "ranker.hpp"
#include <unordered_map>
#include <unordered_set>
namespace comments {
class RecommendationBuffer {
    std::vector<Scored> candidates_, visible_;
    std::unordered_set<std::string> freshHigh_;
    std::unordered_map<std::string, Millis> pinned_;
public:
    void add(const Scored &s);
    void dismiss(const std::string &id);
    std::vector<Scored> update(Millis now, Pick minimum = Pick::Medium, bool hideHostile = true);
};
}
