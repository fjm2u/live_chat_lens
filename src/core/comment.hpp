#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace comments {
using Millis = std::int64_t;
enum class Pick { Low, Medium, High };
enum class Type { Question, Reaction, Joke, Opinion, Other };
struct Comment {
    std::string id, author, text;
    Millis received = 0;
    Millis published = 0;
};
struct Scored {
    Comment comment;
    bool hide = true;
    Pick pick = Pick::Low;
    Type type = Type::Other;
};
const char *name(Pick value);
const char *name(Type value);
}
