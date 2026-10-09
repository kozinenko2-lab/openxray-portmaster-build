#include <cassert>
#include <cstdint>
#include "platform/android_touch_menu.hpp"
int main(){
  AndroidTouchMenu t;
  // A and B must survive down+up between game frames.
  assert(t.frame(0,16,true,0)==16);
  assert(t.frame(0,32,true,16)==32);
  assert(t.frame(0,64,true,33)==64);
  // Flick is a complete press even if released before poll.
  assert(t.frame(0,2,true,50)==2);
  assert(t.frame(0,0,true,66)==0);
  // Held joystick: one initial menu step, no repeated every frame;
  // subsequent deliberate repeat after 340 ms and then each 145 ms.
  assert(t.frame(4,4,true,100)==4);
  assert(t.frame(4,0,true,101)==0);
  assert(t.frame(4,0,true,439)==0);
  assert(t.frame(4,0,true,440)==4);
  assert(t.frame(4,0,true,441)==0);
  assert(t.frame(4,0,true,585)==4);
  assert(t.frame(0,0,true,586)==0);
  assert(t.frame(8,8,true,600)==8);
  // Action presses continue working when a held stick is between pulses.
  assert(t.frame(8|16,16,true,620)==16);
  assert(t.frame(0,0,true,640)==0);
  // Gameplay directions MUST remain continuous, even with button overlays.
  assert(t.frame(1|16,0,false,700)==(1|16));
  assert(t.frame(1|32,0,false,716)==(1|32));
  assert(t.frame(0,16,false,732)==16);
  return 0;
}
