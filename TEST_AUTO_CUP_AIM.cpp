#include <cassert>
#include <cmath>
#include <iostream>
#include "GamePhysics.hpp"
using namespace beg;
static bool approx(float a,float b,float eps=.0001f){return fabsf(a-b)<eps;}
int main(){
 Game g;
 g.setHole(3); // left dogleg cup offset -30
 assert(g.autoAimToCup);
 float cupX=(g.holeX-g.x)/g.distance, cupZ=(g.holeZ-g.z)/g.distance;
 assert(approx(g.aimX,cupX) && approx(g.aimZ,cupZ));
 // Auto tracks the changing cup direction after ball movement.
 g.x=15; g.z=90; g.updateAim();
 assert(approx(g.aimX,(g.holeX-g.x)/g.distance));
 const float x0=g.aimX,z0=g.aimZ;
 g.setAutoAimToCup(false);
 assert(!g.autoAimToCup && approx(g.aimX,x0) && approx(g.aimZ,z0));
 g.x=-20;g.z=180;g.updateAim();
 assert(approx(g.aimX,x0) && approx(g.aimZ,z0));
 assert(approx(g.distance,length2(g.holeX-g.x,g.holeZ-g.z)));
 g.steerAim(1,5);
 float x1=g.aimX,z1=g.aimZ;
 assert(!approx(x1,x0));
 g.x=-45;g.z=260;g.updateAim();
 assert(approx(g.aimX,x1) && approx(g.aimZ,z1));
 // New hole in OFF mode starts straight forward, regardless of cup offset.
 g.setHole(1);g.setAutoAimToCup(false,true);
 assert(approx(g.aimX,0) && approx(g.aimZ,1));
 assert(!g.autoAimToCup);
 g.steerAim(-1,10);
 assert(g.aimX<0);
 // Switching ON aims at cup immediately and remains trackable.
 g.setAutoAimToCup(true);
 assert(g.autoAimToCup && approx(g.aimOffset,0));
 assert(approx(g.aimX,g.holeX/g.distance));
 // OFF retained across a water penalty when the game is configured OFF.
 g.setAutoAimToCup(false);
 g.x=23;g.z=60;g.updateAim();
 const float lockX=g.aimX,lockZ=g.aimZ;
 g.prevShotX=5;g.prevShotZ=7;g.shots=1;g.state=State::SINKING;
 g.waterClock=Game::WATER_VIEW_SECONDS-.01f;
 unsigned ev=g.step(.02f);
 assert((ev&WATER_RESET) && g.shots==3 && g.penaltyStrokes==2);
 assert(g.state==State::READY && approx(g.x,5) && approx(g.z,7));
 assert(approx(g.aimX,lockX) && approx(g.aimZ,lockZ));
 std::cout << "PASS auto cup aim ON/OFF / fixed bearing / manual steering / new hole / water reset\n";
}
