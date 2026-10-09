#include "platform/legacy_text.hpp"
#include <cassert>
#include <iostream>

int main(){
    using namespace LegacyTextInput;
    assert(cp1251FromUtf8("a")==0x61);
    assert(cp1251FromUtf8("A")==0x41);
    assert(cp1251FromUtf8("!")==0x21);
    assert(cp1251FromUtf8("Я")==0xDF);
    assert(cp1251FromUtf8("я")==0xFF);
    assert(cp1251FromUtf8("А")==0xC0);
    assert(cp1251FromUtf8("а")==0xE0);
    assert(cp1251FromUtf8("Ё")==0xA8);
    assert(cp1251FromUtf8("ё")==0xB8);
    assert(cp1251FromUtf8("№")==0xB9);
    assert(cp1251FromUtf8("€")==0x88);
    assert(cp1251FromUtf8("🙂")==-1);
    assert(cp1251FromUtf8("\xC0\xAF")==-1); // overlong/invalid UTF-8
    std::cout << "legacy text r243 PASS\n";
}
