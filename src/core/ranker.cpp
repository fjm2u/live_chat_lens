#include "ranker.hpp"
#include <algorithm>
namespace comments {
std::vector<Scored> rank(std::vector<Scored> items, Millis now, Pick minimum, bool hideHostile) {
    items.erase(std::remove_if(items.begin(), items.end(), [&](const auto &s) {
        return (hideHostile && s.hide) || s.pick < minimum || s.pick == Pick::Low || now - s.comment.received > 60000;
    }), items.end());
    std::stable_sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
        if (a.pick != b.pick) return a.pick > b.pick;
        return a.comment.received > b.comment.received;
    });
    // Break runs within the SAME quality tier and a 10-second freshness window.
    for (size_t i = 2; i < items.size(); ++i) {
        if (items[i-1].type != items[i-2].type || items[i].type != items[i-1].type) continue;
        auto other = std::find_if(items.begin()+i+1, items.end(), [&](const auto &s) {
            return s.pick == items[i].pick && s.type != items[i].type && items[i].comment.received - s.comment.received <= 10000;
        });
        if (other != items.end()) std::rotate(items.begin()+i, other, other+1);
    }
    return items;
}
}
