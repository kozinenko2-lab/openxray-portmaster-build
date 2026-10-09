#include "input.hpp"
#include "platform/legacy_text.hpp"
#include "platform/android_touch_menu.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cctype>
#if defined(__ANDROID__)
#include <atomic>
#include <SDL_system.h>
#include <jni.h>

namespace {
std::atomic<unsigned> axTouchMask{0};
std::atomic<unsigned> axTouchEdges{0};
constexpr unsigned T_UP=1u<<0, T_DOWN=1u<<1, T_LEFT=1u<<2, T_RIGHT=1u<<3,
                   T_ACTION=1u<<4, T_BACK=1u<<5, T_PAUSE=1u<<6;

// Called from the Android UI thread. Snapshot is consumed on SDL game thread.
void vibrateAndroid(float strength, std::uint32_t ms) {
    JNIEnv* e = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!e || !activity) return;
    jclass cls = e->GetObjectClass(activity);
    if (cls) {
        jmethodID m = e->GetMethodID(cls, "vibrateOnDeath", "(II)V");
        if (m) e->CallVoidMethod(activity, m,
             jint(std::clamp(int(strength*255.0f+0.5f),1,255)), jint(ms));
        if (e->ExceptionCheck()) e->ExceptionClear();
        e->DeleteLocalRef(cls);
    }
    e->DeleteLocalRef(activity);
}
}
extern "C" JNIEXPORT void JNICALL
Java_com_airxonix_nativeport_AirXonixActivity_nativeSetPadMask(JNIEnv*,jclass,jint mask) {
    // Retain the DOWN edge even if an Android DOWN+UP happens between SDL frames.
    // Sampling only the currently held mask silently lost fast A/B presses.
    const unsigned next = static_cast<unsigned>(mask)&0x7fu;
    const unsigned prior = axTouchMask.exchange(next, std::memory_order_acq_rel);
    axTouchEdges.fetch_or(next & ~prior, std::memory_order_release);
}
#endif

namespace {
constexpr Sint16 kDeadZone = 6500;
bool axisNegative(Sint16 value) { return value < -kDeadZone; }
bool axisPositive(Sint16 value) { return value >  kDeadZone; }

bool rawButton(SDL_Joystick* joy, int index) {
    return joy && index >= 0 && index < SDL_JoystickNumButtons(joy) && SDL_JoystickGetButton(joy,index) != 0;
}


int legacyCodeFromSDLKey(SDL_Keycode key) {
    if(key>=SDLK_a && key<=SDLK_z) return 0x41 + int(key-SDLK_a);
    if(key>=SDLK_0 && key<=SDLK_9) return 0x30 + int(key-SDLK_0);
    if(key>=SDLK_F1 && key<=SDLK_F9) return 0x70 + int(key-SDLK_F1);
    switch(key){
        case SDLK_TAB:return 0x09;
        case SDLK_LSHIFT:case SDLK_RSHIFT:return 0x10;
        case SDLK_PAGEUP:return 0x21; case SDLK_PAGEDOWN:return 0x22;
        case SDLK_END:return 0x23; case SDLK_HOME:return 0x24;
        case SDLK_INSERT:return 0x2D; case SDLK_DELETE:return 0x2E;
        case SDLK_KP_0:return 0x60; case SDLK_KP_1:return 0x61; case SDLK_KP_2:return 0x62;
        case SDLK_KP_3:return 0x63; case SDLK_KP_4:return 0x64; case SDLK_KP_5:return 0x65;
        case SDLK_KP_6:return 0x66; case SDLK_KP_7:return 0x67; case SDLK_KP_8:return 0x68;
        case SDLK_KP_9:return 0x69; case SDLK_KP_MULTIPLY:return 0x6A; case SDLK_KP_PLUS:return 0x6B;
        case SDLK_KP_MINUS:return 0x6D; case SDLK_KP_PERIOD:return 0x6E; case SDLK_KP_DIVIDE:return 0x6F;
        case SDLK_RETURN:case SDLK_KP_ENTER:return 0x0D;
        case SDLK_ESCAPE:return 0x1B;
        case SDLK_BACKSPACE:return 0x08;
        case SDLK_RIGHT:return 0x27; case SDLK_LEFT:return 0x25; case SDLK_DOWN:return 0x28; case SDLK_UP:return 0x26;
        default:return -1;
    }
}

int legacyJ1ButtonCode(unsigned button) {
    return button<10 ? 0x104 + int(button) : -1; // table labels J1-but1..9,but0
}

bool rawSelectStart(SDL_Joystick* joy) {
    // muOS H700 mapping from PortMaster get_controls: back=b9,start=b10.
    // The other pairs cover common SDL/evdev layouts used by ArkOS/ROCKNIX.
    return (rawButton(joy,9) && rawButton(joy,10)) ||
           (rawButton(joy,8) && rawButton(joy,9)) ||
           (rawButton(joy,6) && rawButton(joy,7));
}
}

InputSystem::InputSystem() {
#if defined(__ANDROID__)
    // SDL_StartTextInput() opens the Android IME. Do not summon a keyboard
    // during startup, menus or gameplay: the name editor is the ONLY owner.
    SDL_StopTextInput();
#else
    SDL_StartTextInput(); // preserve PC/PortMaster physical keyboard behaviour
#endif
    openFirstInputDevice();
}
InputSystem::~InputSystem() { SDL_StopTextInput(); closeDevice(); }

void InputSystem::setRecordNameTextInput(bool enabled) {
#if defined(__ANDROID__)
    // Guard transitions to avoid reopening the soft keyboard every frame.
    if (enabled && !SDL_IsTextInputActive()) SDL_StartTextInput();
    else if (!enabled && SDL_IsTextInputActive()) SDL_StopTextInput();
#else
    (void)enabled;
#endif
}

void InputSystem::closeDevice() {
    if (controller_) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
        joystick_ = nullptr; // controller owned it
    } else if (joystick_ && ownsJoystick_) {
        SDL_JoystickClose(joystick_);
        joystick_ = nullptr;
    }
    ownsJoystick_ = false;
    preferRawDirections_ = false;
    lastRawHatState_ = 0;
}

void InputSystem::openFirstInputDevice() {
    if (joystick_) return;
    const int count = SDL_NumJoysticks();
    std::fprintf(stderr,"AX_INPUT joysticks=%d mapping_env=%s\n",count,std::getenv("SDL_GAMECONTROLLERCONFIG")?"set":"unset");
    int best=-1,bestScore=-100000;
    for (int i=0;i<count;++i) {
        const char* name=SDL_JoystickNameForIndex(i);
        SDL_Joystick* probe=SDL_JoystickOpen(i);
        const int axes=probe?SDL_JoystickNumAxes(probe):0;
        const int buttons=probe?SDL_JoystickNumButtons(probe):0;
        const int hats=probe?SDL_JoystickNumHats(probe):0;
        int score=0;
        if(axes>=2)score+=20;
        if(hats>=1)score+=30;
        if(buttons>=8)score+=20;
        if(SDL_IsGameController(i))score+=10;
        std::string n=name?name:"";
        std::transform(n.begin(),n.end(),n.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(n.find("muos")!=std::string::npos||n.find("deeplay")!=std::string::npos||n.find("gamepad")!=std::string::npos||n.find("keys")!=std::string::npos)score+=50;
        if(n.find("gptk")!=std::string::npos||n.find("virtual")!=std::string::npos)score-=100;
        std::fprintf(stderr,"AX_INPUT device[%d] name=%s gamecontroller=%d axes=%d buttons=%d hats=%d score=%d\n",i,name?name:"<unknown>",SDL_IsGameController(i),axes,buttons,hats,score);
        if(probe)SDL_JoystickClose(probe);
        if(score>bestScore){bestScore=score;best=i;}
    }
    if(best>=0 && SDL_IsGameController(best)){
        controller_=SDL_GameControllerOpen(best);
        if(controller_){
            joystick_=SDL_GameControllerGetJoystick(controller_);ownsJoystick_=false;
            std::string selectedName=SDL_JoystickName(joystick_)?SDL_JoystickName(joystick_):"";
            std::transform(selectedName.begin(),selectedName.end(),selectedName.begin(),[](unsigned char c){return char(std::tolower(c));});
            preferRawDirections_ = selectedName.find("muos")!=std::string::npos ||
                                   selectedName.find("deeplay")!=std::string::npos ||
                                   selectedName.find("keys")!=std::string::npos;
            lastRawHatState_=0;
            std::fprintf(stderr,"AX_INPUT selected gamecontroller index=%d name=%s raw_dirs=%d\n",best,SDL_JoystickName(joystick_),preferRawDirections_?1:0);
            return;
        }
    }
    if(best>=0){
        joystick_=SDL_JoystickOpen(best);
        if(joystick_){
            ownsJoystick_=true;preferRawDirections_=true;lastRawHatState_=0;
            std::fprintf(stderr,"AX_INPUT selected raw joystick index=%d name=%s raw_dirs=1\n",best,SDL_JoystickName(joystick_));return;
        }
    }
    std::fprintf(stderr,"AX_INPUT WARNING no controller/joystick opened: %s\n",SDL_GetError());
}


bool InputSystem::rumble(float strength01,std::uint32_t durationMs) {
    strength01=std::clamp(strength01,0.0f,1.0f);
    if(strength01<=0.0f || durationMs==0u) return false;
#if defined(__ANDROID__)
    vibrateAndroid(strength01,durationMs);
    if (!joystick_) return true;
#else
    if (!joystick_) return false;
#endif
    const Uint16 amplitude=static_cast<Uint16>(strength01*65535.0f+0.5f);
    int rc=-1;
#if SDL_VERSION_ATLEAST(2,0,9)
    if(controller_) rc=SDL_GameControllerRumble(controller_,amplitude,amplitude,durationMs);
    else rc=SDL_JoystickRumble(joystick_,amplitude,amplitude,durationMs);
#else
    (void)amplitude;
#endif
    if(rc!=0){
        static bool warned=false;
        if(!warned){std::fprintf(stderr,"AX_RUMBLE unavailable: %s\n",SDL_GetError());warned=true;}
        return false;
    }
    return true;
}

void InputSystem::poll(InputState& s) {
    const bool oldQuit=s.quit;
    s={};
    s.quit=oldQuit;

    int eventCardinal=0; // 1 up, 2 down, 3 left, 4 right; newest edge wins rollover
    SDL_Event ev;
    while(SDL_PollEvent(&ev)) {
        if(ev.type==SDL_QUIT) s.quit=true;
        if(ev.type==SDL_KEYDOWN && !ev.key.repeat){
            const int code=legacyCodeFromSDLKey(ev.key.keysym.sym);
            if(code>=0){s.legacyPressedCode=code;s.legacyPressedFromController=false;}
        }
        if(ev.type==SDL_TEXTINPUT && s.legacyTextByte<0){
            const int byte=LegacyTextInput::cp1251FromUtf8(ev.text.text);
            if(byte>=0)s.legacyTextByte=byte;
        }
        if(ev.type==SDL_JOYBUTTONDOWN){
            const int code=legacyJ1ButtonCode(ev.jbutton.button);
            if(code>=0){s.legacyPressedCode=code;s.legacyPressedFromController=true;}
        }else if(ev.type==SDL_CONTROLLERBUTTONDOWN){
            s.legacyPressedFromController=true;
            if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT){s.legacyPressedCode=0x100;eventCardinal=4;}
            else if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT){s.legacyPressedCode=0x101;eventCardinal=3;}
            else if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN){s.legacyPressedCode=0x102;eventCardinal=2;}
            else if(ev.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP){s.legacyPressedCode=0x103;eventCardinal=1;}
            else { const int code=legacyJ1ButtonCode(ev.cbutton.button); if(code>=0)s.legacyPressedCode=code; }
        }else if(ev.type==SDL_JOYHATMOTION){
            const Uint8 h=ev.jhat.value;
            if(h&SDL_HAT_UP)eventCardinal=1;
            else if(h&SDL_HAT_DOWN)eventCardinal=2;
            else if(h&SDL_HAT_LEFT)eventCardinal=3;
            else if(h&SDL_HAT_RIGHT)eventCardinal=4;
        }
        if((ev.type==SDL_CONTROLLERDEVICEADDED || ev.type==SDL_JOYDEVICEADDED) && !joystick_) openFirstInputDevice();
        if((ev.type==SDL_CONTROLLERDEVICEREMOVED || ev.type==SDL_JOYDEVICEREMOVED) && joystick_) {
            const SDL_JoystickID current=SDL_JoystickInstanceID(joystick_);
            const SDL_JoystickID removed=(ev.type==SDL_CONTROLLERDEVICEREMOVED)?ev.cdevice.which:ev.jdevice.which;
            if(removed==current){closeDevice();openFirstInputDevice();}
        }
    }

    const Uint8* k=SDL_GetKeyboardState(nullptr);
    // Arrow keys are hard-wired by 0x419270. Letter movement comes only from
    // the configurable legacy bindings, so A/Z/X/C remain usable defaults.
    s.up    =k[SDL_SCANCODE_UP];
    s.down  =k[SDL_SCANCODE_DOWN];
    s.left  =k[SDL_SCANCODE_LEFT];
    s.right =k[SDL_SCANCODE_RIGHT];
    s.action=k[SDL_SCANCODE_RETURN]||k[SDL_SCANCODE_SPACE];
    s.back  =k[SDL_SCANCODE_ESCAPE]||k[SDL_SCANCODE_BACKSPACE];
    s.pause =k[SDL_SCANCODE_P];
    for(int sc=0;sc<SDL_NUM_SCANCODES;++sc){
        if(!k[sc])continue;
        const int code=legacyCodeFromSDLKey(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(sc)));
        if(code>=0 && static_cast<std::size_t>(code)<s.legacyHeld.size())s.legacyHeld[static_cast<std::size_t>(code)]=true;
    }

    bool selectPressed=false,startPressed=false;
    if(controller_) {
        const Sint16 ax=SDL_GameControllerGetAxis(controller_,SDL_CONTROLLER_AXIS_LEFTX);
        const Sint16 ay=SDL_GameControllerGetAxis(controller_,SDL_CONTROLLER_AXIS_LEFTY);
        if(!preferRawDirections_){
            s.left |=axisNegative(ax)||SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            s.right|=axisPositive(ax)||SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
            s.up   |=axisNegative(ay)||SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_UP);
            s.down |=axisPositive(ay)||SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        }
        s.legacyHeld[0x100]=s.right; s.legacyHeld[0x101]=s.left;
        s.legacyHeld[0x102]=s.down;  s.legacyHeld[0x103]=s.up;
        // PortMaster face-button contract: A confirms. X is intentionally NOT an alias\n        // for confirm; mapping it to action made a single X press start a new game.\n        // RG40XX-H/muOS exposes the same physical pad both through the SDL
        // GameController mapping and the underlying joystick.  On this firmware
        // A is physical raw button 3 (SDL_GAMECONTROLLERCONFIG: a:b3).  Some
        // SDL builds intermittently fail to reflect that state through
        // SDL_GameControllerGetButton even while axes/D-pad continue to work.
        // Accept raw b3 as an A-only hardware fallback.  Do NOT restore raw b6:
        // that is physical X and was the r167 bug that started a game on X.
#if defined(__ANDROID__)
        s.action|=SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_A);
        s.back|=SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_B);
#else
        s.action|=SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_A) || rawButton(joystick_,3);
        s.back|=SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_B) || rawButton(joystick_,4);
#endif
        startPressed=SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_START)!=0;
        selectPressed=SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_BACK)!=0;
        s.pause|=startPressed;
        s.select|=selectPressed;
    }

    // Always read the underlying raw joystick as a fallback. Some PortMaster
    // SDL builds expose D-pad only as hat0 even when GameController opens.
    if(joystick_) {
        SDL_JoystickUpdate();
        Sint16 ax=0,ay=0;
        if(SDL_JoystickNumAxes(joystick_)>0) ax=SDL_JoystickGetAxis(joystick_,0);
        if(SDL_JoystickNumAxes(joystick_)>1) ay=SDL_JoystickGetAxis(joystick_,1);
        Uint8 hat=SDL_HAT_CENTERED;
        if(SDL_JoystickNumHats(joystick_)>0) hat=SDL_JoystickGetHat(joystick_,0);
        if(!controller_ || preferRawDirections_){
            // H700/muOS exposes the D-pad and left stick through the same raw
            // joystick.  The firmware can leave noisy/stale axis values while
            // hat0 is held.  Original AirXonix is strictly cardinal, so make
            // HAT absolute priority: while it is non-centred, ignore axes.
            // Only when HAT is centred do we read the stick, and then choose
            // one dominant axis instead of manufacturing a diagonal.
            if(hat!=SDL_HAT_CENTERED){
                s.left =(hat&SDL_HAT_LEFT)!=0; s.right=(hat&SDL_HAT_RIGHT)!=0;
                s.up   =(hat&SDL_HAT_UP)!=0;   s.down =(hat&SDL_HAT_DOWN)!=0;
            }else{
                const int iax=int(ax), iay=int(ay);
                if(std::abs(iax)>kDeadZone || std::abs(iay)>kDeadZone){
                    if(std::abs(iax)>std::abs(iay)) { s.left=iax<0; s.right=iax>0; }
                    else { s.up=iay<0; s.down=iay>0; }
                }
            }

            const unsigned rawHat=unsigned(hat)&0x0fu;
            const unsigned newlyPressed=rawHat & ~lastRawHatState_;
            auto cardinalFromBits=[](unsigned bits)->int{
                if(bits&SDL_HAT_UP)return 1;
                if(bits&SDL_HAT_DOWN)return 2;
                if(bits&SDL_HAT_LEFT)return 3;
                if(bits&SDL_HAT_RIGHT)return 4;
                return 0;
            };
            const int edge=cardinalFromBits(newlyPressed);
            if(edge!=0)eventCardinal=edge;
            lastRawHatState_=rawHat;
        }else{
            // Generic GameController path: use the raw hat only when SDL's
            // semantic D-pad is completely neutral.
            const bool gcDpad =
                SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_LEFT)||
                SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_RIGHT)||
                SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_UP)||
                SDL_GameControllerGetButton(controller_,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            if(!gcDpad){
                s.left |=(hat&SDL_HAT_LEFT)!=0; s.right|=(hat&SDL_HAT_RIGHT)!=0;
                s.up   |=(hat&SDL_HAT_UP)!=0;   s.down |=(hat&SDL_HAT_DOWN)!=0;
            }
            lastRawHatState_=unsigned(hat)&0x0fu;
        }
        // r173 hardware contract from the RG40XX-H/muOS trace (r79): raw b9
        // is Select/Back and raw b10 is Start.  Apply these even when SDL has
        // opened a GameController: on this firmware BACK/START semantic states
        // can stay false while the underlying raw buttons are correct.
        selectPressed |= rawButton(joystick_,9);
        startPressed  |= rawButton(joystick_,10);
        s.select |= selectPressed;
        s.pause  |= startPressed;

        s.legacyHeld[0x100]=s.legacyHeld[0x100]||s.right;
        s.legacyHeld[0x101]=s.legacyHeld[0x101]||s.left;
        s.legacyHeld[0x102]=s.legacyHeld[0x102]||s.down;
        s.legacyHeld[0x103]=s.legacyHeld[0x103]||s.up;
        for(int i=0;i<std::min(SDL_JoystickNumButtons(joystick_),10);++i)
            if(SDL_JoystickGetButton(joystick_,i))s.legacyHeld[static_cast<std::size_t>(0x104+i)]=true;

        if(!controller_) {
            // muOS/PortMaster Deeplay/muOS-Keys raw fallback.
            s.action|=rawButton(joystick_,3)||rawButton(joystick_,6)||rawButton(joystick_,0);
            s.back  |=rawButton(joystick_,4)||rawButton(joystick_,1);
            startPressed|=rawButton(joystick_,10);
            selectPressed|=rawButton(joystick_,9);
            s.pause|=startPressed;
            s.select|=selectPressed;
        }

        // Low-volume hardware trace: log only when values materially change.
        const int qx=(std::abs(int(ax))>kDeadZone)?(ax<0?-1:1):0;
        const int qy=(std::abs(int(ay))>kDeadZone)?(ay<0?-1:1):0;
        if(int(hat)!=lastLoggedHat_ || qx!=lastLoggedAxisX_ || qy!=lastLoggedAxisY_) {
            std::fprintf(stderr,"AX_INPUT state axis0=%d axis1=%d qx=%d qy=%d hat0=0x%02x\n",int(ax),int(ay),qx,qy,unsigned(hat));
            lastLoggedHat_=int(hat);lastLoggedAxisX_=qx;lastLoggedAxisY_=qy;
        }
        unsigned mask=0;
        const int nb=std::min(SDL_JoystickNumButtons(joystick_),16);
        for(int i=0;i<nb;++i)if(SDL_JoystickGetButton(joystick_,i))mask|=1u<<i;
        if(mask!=loggedButtonMask_){std::fprintf(stderr,"AX_INPUT buttons=0x%04x\n",mask);loggedButtonMask_=mask;}
    }

    s.select|=selectPressed;

#if defined(__ANDROID__)
    // Multi-touch overlay is platform-only and never changes PC/PortMaster input.
    const unsigned held = axTouchMask.load(std::memory_order_acquire);
    const unsigned fresh = axTouchEdges.exchange(0, std::memory_order_acq_rel);
    const unsigned touch = androidTouchMenu_.frame(held, fresh, menuTouchNavigation_, SDL_GetTicks());
    // Touch control acts as a physical keyboard: arrows/Enter/Escape.
    // Bluetooth/USB gamepads retain their separate J1 key mapping.
    AndroidTouchKeyboard::apply(s,touch,fresh,menuTouchNavigation_);
    if(touch & AndroidTouchKeyboard::Directions) {
        if(touch&AndroidTouchKeyboard::Up) eventCardinal=1;
        else if(touch&AndroidTouchKeyboard::Down) eventCardinal=2;
        else if(touch&AndroidTouchKeyboard::Left) eventCardinal=3;
        else if(touch&AndroidTouchKeyboard::Right) eventCardinal=4;
    }
#endif

    // AirXonix movement is strictly cardinal. Resolve D-pad rollover using the
    // newest edge event, and analog diagonals by dominant axis. Do not keep an
    // old direction merely because a firmware reports it for one extra poll.
    const int directionalCount=int(s.up)+int(s.down)+int(s.left)+int(s.right);
    if(directionalCount>1){
        int keep=eventCardinal;
        if(keep==0 && controller_ && !preferRawDirections_){
            const Sint16 ax=SDL_GameControllerGetAxis(controller_,SDL_CONTROLLER_AXIS_LEFTX);
            const Sint16 ay=SDL_GameControllerGetAxis(controller_,SDL_CONTROLLER_AXIS_LEFTY);
            if(std::abs(int(ax))>kDeadZone || std::abs(int(ay))>kDeadZone){
                if(std::abs(int(ax))>std::abs(int(ay))) keep=ax<0?3:4;
                else keep=ay<0?1:2;
            }
        }
        if(keep==0){
            // No fresh rollover edge: retain the already chosen direction while
            // that cardinal is still physically held. Choosing a *different*
            // direction here (r168/r169 behaviour) caused visible pauses when
            // muOS briefly reported a two-bit hat state.
            if(lastCardinalDirection_==1 && s.up)keep=1;
            else if(lastCardinalDirection_==2 && s.down)keep=2;
            else if(lastCardinalDirection_==3 && s.left)keep=3;
            else if(lastCardinalDirection_==4 && s.right)keep=4;
            else if(s.up)keep=1; else if(s.down)keep=2; else if(s.left)keep=3; else keep=4;
        }
        s.up=keep==1;s.down=keep==2;s.left=keep==3;s.right=keep==4;
    }
    if(s.up)lastCardinalDirection_=1;
    else if(s.down)lastCardinalDirection_=2;
    else if(s.left)lastCardinalDirection_=3;
    else if(s.right)lastCardinalDirection_=4;
    else lastCardinalDirection_=0;
#if !defined(__ANDROID__)
    s.legacyHeld[0x100]=s.right;
    s.legacyHeld[0x101]=s.left;
    s.legacyHeld[0x102]=s.down;
    s.legacyHeld[0x103]=s.up;
#endif

    // PortMaster convention. This is native and does not depend on gptokeyb.
    if((selectPressed&&startPressed)||rawSelectStart(joystick_)) {
        std::fprintf(stderr,"AX_INPUT hotkey Select+Start -> quit\n");
        s.quit=true;
    }
}
