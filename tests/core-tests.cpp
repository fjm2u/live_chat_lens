#include "core/filter.hpp"
#include "core/recommendation-buffer.hpp"
#include <iostream>
#include <stdexcept>
using namespace comments;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
Scored sample(std::string id,Millis time=1000,Pick pick=Pick::High,Type type=Type::Question,bool hide=false) {
    return {{id,"author-"+id,"comment-"+id,time},hide,pick,type};
}
int main() {
    BasicFilter f;
    CHECK(!f.accept({"a","u","",0})); CHECK(!f.accept({"a","u","  ",0}));
    CHECK(!f.accept({"a","u","https://example.com",0}));
    CHECK(!f.accept({"a","u",std::string(2001,'a'),0}));
    CHECK(f.accept({"a","u","one",1})); CHECK(!f.accept({"b","v","one",2}));
    CHECK(f.accept({"b","u","two",2})); CHECK(f.accept({"c","u","three",3}));
    CHECK(!f.accept({"d","u","four",4})); CHECK(f.accept({"d","u","four",6000}));
    CHECK(f.accept({"e","v","one",61002}));
    auto ranked=rank({sample("hidden",1001,Pick::High,Type::Question,true),sample("low",1002,Pick::Low),sample("mid",1010,Pick::Medium),sample("high",1000)},1100,Pick::Medium,true);
    CHECK(ranked.size()==2); CHECK(ranked[0].comment.id=="high");
    CHECK(rank({sample("old",0)},61000,Pick::Medium,true).empty());
    auto diverse=rank({sample("q1",1005),sample("q2",1004),sample("q3",1003),sample("op",1002,Pick::High,Type::Opinion)},1010,Pick::Medium,true);
    CHECK(diverse[2].type==Type::Opinion);
    auto quality=rank({sample("q1",1005),sample("q2",1004),sample("q3",1003),sample("op",1002,Pick::Medium,Type::Opinion)},1010,Pick::Medium,true);
    CHECK(quality[2].pick==Pick::High);
    RecommendationBuffer b; b.add(sample("first",1000,Pick::Medium));
    CHECK(b.update(1000)[0].comment.id=="first");
    b.add(sample("second",1100,Pick::Medium)); CHECK(b.update(1100)[0].comment.id=="first");
    CHECK(b.update(3999)[0].comment.id=="first"); CHECK(b.update(4101)[0].comment.id=="second");
    b.add(sample("high",4200)); CHECK(b.update(4200)[0].comment.id=="high");
    b.dismiss("high"); for (auto &s:b.update(4201)) CHECK(s.comment.id!="high");
    b.add(sample("hostile",4202,Pick::High,Type::Other,true));
    for (auto &s:b.update(4203)) CHECK(!s.hide);
    CHECK(b.update(70000).empty());
    for (int i=0;i<1000;++i) b.add(sample(std::to_string(i),70000+i));
    CHECK(b.update(71000).size()==10);
    std::cout<<"core: filter, TTL, ranking, diversity, stability, high insertion, dismiss, bounds PASS\n";
}
