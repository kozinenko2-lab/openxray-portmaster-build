#include "game/game.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
struct GameTestProbe {
    static void finale(Game& g){g.beginFinalSequence();}
    static void settingsRow(Game& g,int row){g.phase_=GamePhase::Settings;g.settings_.selected=row;g.settings_.selectorOffset=LegacySettingsTrace::selectorTarget(row>=6?0:row);g.settings_.testLrLatched=false;g.settings_.testRepeatMs=0;g.settings_.fadeCounter=LegacySettingsTrace::fadeMax;g.settings_.readyForInput=true;g.settings_.navLatched=false;}
};
int main(){
    { Game g; GameTestProbe::finale(g); const float p=g.displayPlayerPropellerPhase(); g.update(InputState{},100); if(std::fabs(g.displayPlayerPropellerPhase()-p)<1e-5f){std::cerr<<"finale propeller did not move\n";return 1;} }
    { Game g; GameTestProbe::settingsRow(g,6); InputState in{};in.right=true; g.update(in,16); const int first=g.settings().testInitialLives; if(first!=4)return 2; for(int i=0;i<30;++i)g.update(in,16); if(g.settings().testInitialLives<=first){std::cerr<<"held D-pad did not repeat lives\n";return 3;} }
    { Game g; GameTestProbe::settingsRow(g,7); InputState in{};in.right=true; g.update(in,16); const int first=g.settings().testInitialTimeSeconds; for(int i=0;i<30;++i)g.update(in,16); if(first!=70||g.settings().testInitialTimeSeconds<=first)return 4; }
    {
        std::ifstream f(std::string(AIRXONIX_SOURCE_DIR)+"/portmaster/AirXonix.sh");
        const std::string sh((std::istreambuf_iterator<char>(f)),{});
        if(sh.find("VIBRATION_ENABLED") == std::string::npos ||
           sh.find("VIBRATION_DURATION_MS") == std::string::npos ||
           sh.find("VIBRATION_STRENGTH") == std::string::npos) return 5;
    }
    return 0;
}
