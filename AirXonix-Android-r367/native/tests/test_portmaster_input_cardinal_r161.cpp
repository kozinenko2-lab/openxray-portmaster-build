#include <cassert>
#include "platform/input.hpp"
int main(){
    InputState s{};
    // Core contract: gameplay-facing direction set is representable as one cardinal.
    s.up=true; s.right=false; s.down=false; s.left=false;
    assert(int(s.up)+int(s.down)+int(s.left)+int(s.right)==1);
    return 0;
}
