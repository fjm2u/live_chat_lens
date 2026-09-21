#pragma once
#include "comment.hpp"
#include <deque>
namespace comments {
class BasicFilter {
    std::deque<Comment> recent_;
public:
    // Input is normalized/trimmed UTF-8 by the receiver. Bounded recent history.
    bool accept(const Comment &c);
};
}
