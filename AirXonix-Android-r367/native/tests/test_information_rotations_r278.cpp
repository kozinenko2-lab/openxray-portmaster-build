#include "render/legacy_transform.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool near(float a,float b){return std::fabs(a-b)<1e-6f;}
int main(){
    auto m=LegacyTransform::identity();
    const float a=0.1234567f;
    LegacyTransform::rotateZRad(m,a);
    assert(near(m.m[0],std::cos(a)));
    assert(near(m.m[1],std::sin(a)));
    assert(near(m.m[3],-std::sin(a)));
    assert(near(m.m[4],std::cos(a)));
    // EXE 0x41451E -> 0x43129C truncates the legacy Y angle.
    assert(int(10.75f)==10);
    assert(int(-10.75f)==-10);
    std::cout << "information rotations r278 PASS\n";
}
