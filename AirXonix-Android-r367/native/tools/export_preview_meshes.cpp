#include "render/legacy_model_factory.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

static void writeObj(const std::string& path,const LegacyMesh& m){
 std::ofstream o(path); for(auto& v:m.vertices)o<<"v "<<v.x<<' '<<v.y<<' '<<v.z<<"\n";
 for(auto& v:m.vertices)o<<"vt "<<v.u<<' '<<v.v<<"\n";
 for(auto& f:m.faces){o<<"f"; for(auto i:f.index)o<<' '<<(i+1)<<'/'<<(i+1); o<<"\n";}
}
int main(int argc,char**argv){std::string d=argc>1?argv[1]:"/mnt/data/airx_meshes";std::filesystem::create_directories(d);
 auto x=LegacyModelFactory::buildXonix(true);writeObj(d+"/xonix_body.obj",x.body);writeObj(d+"/xonix_propeller.obj",x.propeller);writeObj(d+"/xonix_rotornode.obj",x.rotorNode);
 writeObj(d+"/ground_enemy.obj",LegacyModelFactory::buildGroundEnemy(true));
 for(int i=0;i<4;++i)writeObj(d+"/air_enemy"+std::to_string(i)+".obj",LegacyModelFactory::buildAirEnemySubtype(i,true));
 writeObj(d+"/air_shadow.obj",LegacyModelFactory::buildAirEnemyShadow());
 for(int i=0;i<6;++i)writeObj(d+"/pickup"+std::to_string(i)+".obj",LegacyModelFactory::buildPickupType(i,true));
 auto h=LegacyModelFactory::buildHoming(true);writeObj(d+"/homing_body.obj",h.body);writeObj(d+"/homing_dome.obj",h.dome);
 auto e=LegacyModelFactory::buildEraser(true);writeObj(d+"/eraser_star.obj",e.star);writeObj(d+"/eraser_core.obj",e.core);
 writeObj(d+"/trail.obj",LegacyModelFactory::buildTrailSegment(false,true));
 return 0;}
