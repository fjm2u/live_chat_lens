#include "comment.hpp"
namespace comments {
const char *name(Pick v) { return v == Pick::High ? "high" : v == Pick::Medium ? "medium" : "low"; }
const char *name(Type v) {
    switch (v) {
    case Type::Question: return "question";
    case Type::Reaction: return "reaction";
    case Type::Joke: return "joke";
    case Type::Opinion: return "opinion";
    default: return "other";
    }
}
}
