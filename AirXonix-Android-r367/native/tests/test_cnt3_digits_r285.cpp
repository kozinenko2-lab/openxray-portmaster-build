#include "render/legacy_cnt3_digits.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool nearf(float a,float b,float e=1e-8f){return std::fabs(a-b)<=e;}
int main(){
    using namespace LegacyCnt3Digits;
    auto one=build(7,1);
    assert(one.vertices.size()==4 && one.quads.size()==1);
    assert(nearf(one.vertices[0].x,0.f) && nearf(one.vertices[2].x,Width));
    assert(nearf(one.vertices[0].u,7.f*UStep));
    assert(nearf(one.vertices[2].u,8.f*UStep));
    assert(nearf(one.vertices[0].v,VBottom) && nearf(one.vertices[1].v,VTop));
    assert(nearf(one.vertices[0].materialOrLight,Light));
    auto two=build(12,2);
    assert(two.vertices.size()==8 && two.quads.size()==2);
    assert(nearf(two.vertices[0].u,UStep));
    assert(nearf(two.vertices[4].u,2.f*UStep));
    assert(nearf(two.vertices[4].x,Width) && nearf(two.vertices[6].x,2.f*Width));
    std::cout << "cnt3 digits r285 PASS\n";
}
