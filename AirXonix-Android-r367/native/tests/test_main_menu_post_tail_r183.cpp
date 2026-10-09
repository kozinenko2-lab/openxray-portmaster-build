#include "render/legacy_main_menu_decor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool nearf(float a,float b){return std::fabs(a-b)<1e-7f;}
int main(){
    using T=LegacyMainMenuDecorationTrace;
    assert(nearf(T::selectorX,0.009f));
    assert(nearf(T::selectorScale,0.2f));
    assert(T::selectorLegacyUnitsPerMs==2);
    assert(nearf(T::selectorZForIndex(0),-0.0015f));
    assert(nearf(T::selectorZForIndex(1),-0.0045f));
    assert(nearf(T::selectorZForIndex(2),-0.0075f));
    assert(nearf(T::selectorZForIndex(3),-0.0105f));
    assert(nearf(T::selectorZForIndex(4),-0.0135f));
    std::cout << "main menu post-tail r183 ok\
";
}
