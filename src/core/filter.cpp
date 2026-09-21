#include "filter.hpp"
#include <algorithm>
#include <regex>
namespace comments {
bool BasicFilter::accept(const Comment &c) {
    while (!recent_.empty() && (c.received - recent_.front().received > 60000 || recent_.size() >= 2000)) recent_.pop_front();
    if (c.id.empty() || c.author.empty() || c.text.empty() || c.text.size() > 2000 ||
        c.text.find_first_not_of(" \t\r\n") == std::string::npos) return false;
    static const std::regex url(R"(^\s*(?:(?:https?://|www\.)\S+\s*)+$)", std::regex::icase);
    if (std::regex_match(c.text, url)) return false;
    int burst = 0;
    for (const auto &r : recent_) {
        if (r.id == c.id || r.text == c.text) return false;
        if (r.author == c.author && c.received - r.received < 5000) ++burst;
    }
    if (burst >= 3) return false;
    recent_.push_back(c);
    return true;
}
}
