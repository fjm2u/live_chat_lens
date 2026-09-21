#include "recommendation-buffer.hpp"
#include <algorithm>
#include <array>
#include <optional>
#include <unordered_set>
namespace comments {
void RecommendationBuffer::add(const Scored &s) {
    if (std::any_of(candidates_.begin(), candidates_.end(), [&](const auto &x){ return x.comment.id == s.comment.id; })) return;
    candidates_.push_back(s);
    if (s.pick==Pick::High) freshHigh_.insert(s.comment.id);
    if (candidates_.size() > 200) { freshHigh_.erase(candidates_.front().comment.id); candidates_.erase(candidates_.begin()); }
}
void RecommendationBuffer::dismiss(const std::string &id) {
    const auto match = [&](const auto &x){ return x.comment.id == id; };
    candidates_.erase(std::remove_if(candidates_.begin(), candidates_.end(), match), candidates_.end());
    visible_.erase(std::remove_if(visible_.begin(), visible_.end(), match), visible_.end());
    pinned_.erase(id);
}
std::vector<Scored> RecommendationBuffer::update(Millis now, Pick minimum, bool hideHostile) {
    candidates_.erase(std::remove_if(candidates_.begin(), candidates_.end(), [&](const auto &s){ return now-s.comment.received > 60000; }), candidates_.end());
    auto ordered = rank(candidates_, now, minimum, hideHostile);
    std::array<std::optional<Scored>,10> slots;
    std::unordered_set<std::string> used;
    for (size_t i=0; i<visible_.size(); ++i) {
        const auto &s = visible_[i];
        if (now - pinned_[s.comment.id] < 3000 && std::any_of(ordered.begin(),ordered.end(),[&](const auto &x){return x.comment.id == s.comment.id;})) {
            slots[i] = s; used.insert(s.comment.id);
        }
    }
    for (const auto &s : ordered) {
        if (used.count(s.comment.id)) continue;
        auto free = std::find_if(slots.begin(),slots.end(),[](const auto &x){return !x;});
        if (free == slots.end()) break;
        *free = s; used.insert(s.comment.id);
    }
    std::vector<Scored> next;
    for (const auto &slot : slots) if (slot) next.push_back(*slot);
    // Explicit stability exception: newly arrived high comments may enter at the top.
    // Limit to three per tick to avoid replacing the whole reading surface.
    std::vector<Scored> arrivals;
    for (const auto &s:ordered) {
        if (s.pick!=Pick::High || !freshHigh_.count(s.comment.id) || arrivals.size()>=3 ||
            std::any_of(visible_.begin(),visible_.end(),[&](const auto &v){return v.comment.id==s.comment.id;})) continue;
        arrivals.push_back(s);
    }
    for (auto it=arrivals.rbegin(); it!=arrivals.rend(); ++it) {
        const auto &s=*it;
        next.erase(std::remove_if(next.begin(),next.end(),[&](const auto &v){return v.comment.id==s.comment.id;}),next.end());
        next.insert(next.begin(),s);
        if (next.size()>10) next.resize(10);
    }
    freshHigh_.clear();
    used.clear(); for (const auto &s:next) used.insert(s.comment.id);
    // Pin again after a position changes, to keep each new reading position stable.
    for (size_t i=0; i<next.size(); ++i)
        if (i>=visible_.size() || visible_[i].comment.id != next[i].comment.id) pinned_[next[i].comment.id] = now;
    for (auto it=pinned_.begin(); it!=pinned_.end();) {
        if (!used.count(it->first)) it=pinned_.erase(it); else ++it;
    }
    visible_=next;
    return visible_;
}
}
