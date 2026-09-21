#pragma once
#include "comment.hpp"
namespace comments {
std::vector<Scored> rank(std::vector<Scored> candidates, Millis now, Pick minimum, bool hideHostile);
}
