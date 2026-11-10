#pragma once
// BALL'S EYE GOLF - platform independent, deterministic 3D ball simulation.
// Coordinates: X sideways, Z down the fairway, Y vertical; metres and seconds.
#include <math.h>
#include <stdint.h>
#include <stddef.h>

namespace beg {

inline float clampf(float x, float low, float high) {
    return x < low ? low : (x > high ? high : x);
}
inline float length2(float x, float z) { return sqrtf(x*x + z*z); }

enum class State : uint8_t { READY, AIRBORNE, ROLLING, HOLED, SINKING };
enum Event : uint8_t { NONE = 0, IMPACT = 1, LANDING = 2, CUP = 4, STOP = 8, WATER_HIT = 16, WATER_RESET = 32, RAVINE_HIT = 64, TREE_HIT = 128 };

// R12: Nine independently authored courses. All coordinates are in metres.
// The same per-hole data drives terrain drawing, the mini map, and collision.
struct HazardEllipse { float x,z,rx,rz; };
struct FairwayNode { float z,x,halfWidth; };
struct TreeMark { float x,z; };
enum CourseTerrain : uint8_t { LINKS=0, WATER=1, RAVINE=2, WOODS=3, SAND=4 };
struct HoleLayout {
    const char* name;
    float distance;
    uint8_t par;
    float cupX;
    uint8_t terrain;
    uint8_t fairwayCount;
    FairwayNode path[6];
    uint8_t bunkerCount;
    HazardEllipse bunkers[6];
    uint8_t pondCount;
    HazardEllipse ponds[4];
    uint8_t treeCount;
    TreeMark trees[14];
    float ravineStart, ravineEnd;
};
static constexpr HoleLayout kNineHoles[9] = {
  {"STRAIGHT", 92.0f, 3, 0.0f, 0, 6, {{-10.0f,0.0f,12.0f},{0.0f,0.0f,13.0f},{20.0f,1.0f,12.0f},{46.0f,0.0f,11.0f},{72.0f,0.0f,12.0f},{106.0f,0.0f,13.0f}}, 3, {{13.0f,65.0f,6.0f,9.0f},{-16.5f,74.0f,8.0f,7.0f},{12.5f,35.0f,8.0f,7.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 2, {{-10.0f,50.0f,9.0f,11.0f},{13.5f,78.0f,9.5f,8.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 10, {{-23.0f,105.0f},{26.0f,101.0f},{-25.0f,89.0f},{26.0f,80.0f},{-24.0f,70.0f},{26.0f,59.0f},{-23.0f,47.0f},{25.0f,34.0f},{-27.0f,23.0f},{28.0f,12.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}}, 0.0f, 0.0f},
  {"RIGHT DOGLEG",245.0f,4,24.0f,0,6,{{-10.0f,0.0f,13.0f},{0.0f,0.0f,13.0f},{56.06f,0.0f,12.0f},{126.65f,7.0f,11.0f},{188.94f,23.0f,11.0f},{245.0f,24.0f,12.0f}},3,{{12.0f,83.05f,8.0f,8.36f},{-12.0f,149.49f,8.0f,9.55f},{30.0f,218.01f,7.0f,10.74f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},1,{{-16.0f,114.19f,9.0f,14.32f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},10,{{-25.0f,24.92f},{26.0f,33.22f},{-25.0f,83.05f},{29.0f,93.43f},{-23.0f,137.03f},{32.0f,141.19f},{-14.0f,182.71f},{43.0f,182.71f},{2.0f,228.39f},{45.0f,238.77f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}},0.0f,0.0f},
  {"WATER CARRY", 76.0f, 3, -8.0f, 1, 6, {{-10.0f,0.0f,11.0f},{0.0f,0.0f,11.0f},{20.0f,-1.0f,10.0f},{37.0f,-2.0f,7.0f},{61.0f,-8.0f,9.0f},{89.0f,-8.0f,11.0f}}, 3, {{7.0f,27.0f,7.0f,8.0f},{-22.0f,58.0f,8.0f,7.0f},{-13.0f,72.0f,5.0f,5.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 2, {{0.0f,43.0f,23.0f,11.0f},{18.0f,68.0f,8.0f,8.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 8, {{-22.0f,14.0f},{22.0f,14.0f},{-25.0f,30.0f},{26.0f,29.0f},{-30.0f,57.0f},{28.0f,59.0f},{-28.0f,75.0f},{20.0f,81.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}}, 0.0f, 0.0f},
  {"LEFT DOGLEG",315.0f,4,-30.0f,0,6,{{-10.0f,0.0f,13.0f},{0.0f,0.0f,13.0f},{64.85f,0.0f,12.0f},{152.87f,-9.0f,11.0f},{233.93f,-30.0f,10.0f},{349.74f,-30.0f,12.0f}},3,{{11.0f,136.65f,8.0f,12.37f},{-25.0f,210.77f,7.0f,9.9f},{-40.0f,268.68f,9.0f,9.9f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},1,{{18.0f,189.93f,10.0f,17.32f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},10,{{-24.0f,34.74f},{27.0f,44.01f},{-27.0f,99.6f},{27.0f,115.81f},{-38.0f,162.13f},{25.0f,192.24f},{-47.0f,236.25f},{-3.0f,270.99f},{-48.0f,296.47f},{3.0f,310.37f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}},0.0f,0.0f},
  {"RAVINE CROSS", 102.0f, 4, 3.0f, 2, 6, {{-10.0f,0.0f,12.0f},{0.0f,0.0f,12.0f},{23.0f,0.0f,11.0f},{42.0f,1.0f,11.0f},{73.0f,3.0f,10.0f},{116.0f,3.0f,12.0f}}, 3, {{14.0f,26.0f,6.0f,8.0f},{-15.0f,74.0f,7.0f,10.0f},{14.0f,95.0f,6.0f,8.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 0, {{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 8, {{-25.0f,17.0f},{24.0f,19.0f},{-30.0f,35.0f},{29.0f,35.0f},{-28.0f,66.0f},{29.0f,68.0f},{-24.0f,91.0f},{29.0f,96.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}}, 40.0f, 58.0f},
  {"LAKESIDE",420.0f,5,14.0f,1,6,{{-10.0f,0.0f,14.0f},{0.0f,0.0f,14.0f},{95.45f,8.0f,12.0f},{215.45f,18.0f,12.0f},{330.0f,14.0f,12.0f},{455.45f,14.0f,14.0f}},3,{{33.0f,201.82f,8.0f,15.73f},{4.0f,302.73f,8.0f,13.11f},{25.0f,379.09f,7.0f,10.49f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},2,{{-9.0f,174.55f,14.0f,44.57f},{-4.0f,327.27f,13.0f,31.46f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},8,{{-27.0f,49.09f},{29.0f,54.55f},{-31.0f,120.0f},{34.0f,150.0f},{-31.0f,234.55f},{36.0f,259.09f},{-27.0f,346.36f},{37.0f,387.27f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}},0.0f,0.0f},
  {"WOODLAND", 87.0f, 3, 0.0f, 3, 6, {{-10.0f,0.0f,10.0f},{0.0f,0.0f,9.0f},{22.0f,5.0f,8.0f},{45.0f,-4.0f,7.0f},{69.0f,0.0f,8.0f},{100.0f,0.0f,10.0f}}, 2, {{13.0f,44.0f,5.0f,8.0f},{-12.0f,71.0f,5.0f,6.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 0, {{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}}, 14, {{-14.0f,12.0f},{15.0f,12.0f},{-16.0f,27.0f},{19.0f,27.0f},{-13.0f,43.0f},{15.0f,43.0f},{-17.0f,57.0f},{16.0f,59.0f},{-12.0f,76.0f},{13.0f,77.0f},{-29.0f,35.0f},{28.0f,44.0f},{-28.0f,70.0f},{28.0f,75.0f}}, 0.0f, 0.0f},
  {"SAND MAZE",260.0f,4,-3.0f,4,6,{{-10.0f,0.0f,13.0f},{0.0f,0.0f,13.0f},{54.52f,0.0f,12.0f},{121.61f,-6.0f,10.0f},{195.0f,-3.0f,11.0f},{287.26f,-3.0f,12.0f}},6,{{8.0f,46.13f,6.0f,10.78f},{-10.0f,85.97f,7.0f,9.58f},{9.0f,127.9f,7.0f,10.78f},{-18.0f,167.74f,6.0f,9.58f},{8.0f,211.77f,7.0f,9.58f},{-12.0f,245.32f,5.0f,7.18f}},0,{{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},8,{{-26.0f,25.16f},{27.0f,29.35f},{-27.0f,73.39f},{25.0f,90.16f},{-28.0f,153.06f},{27.0f,161.45f},{-27.0f,220.16f},{25.0f,243.23f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}},0.0f,0.0f},
  {"S-CURVE FINISH",480.0f,5,-20.0f,1,6,{{-10.0f,0.0f,14.0f},{0.0f,0.0f,14.0f},{111.02f,13.0f,12.0f},{228.57f,-10.0f,11.0f},{346.12f,-20.0f,11.0f},{525.71f,-20.0f,13.0f}},5,{{24.0f,101.22f,8.0f,11.2f},{-18.0f,218.78f,8.0f,14.0f},{4.0f,316.73f,7.0f,11.2f},{-32.0f,414.69f,7.0f,11.2f},{-6.0f,457.14f,6.0f,9.8f},{0.0f,0.0f,0.0f,0.0f}},2,{{-14.0f,150.2f,10.0f,16.8f},{14.0f,271.02f,11.0f,18.2f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f,0.0f}},10,{{-26.0f,52.24f},{28.0f,58.78f},{-8.0f,130.61f},{35.0f,146.94f},{-30.0f,238.37f},{27.0f,238.37f},{-37.0f,323.27f},{8.0f,355.92f},{-39.0f,424.49f},{5.0f,453.88f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f},{0.0f,0.0f}},0.0f,0.0f}
};
static constexpr int kHoleCount = 9;
// Fixed storage limits, NOT the number of active hazards in every hole.
static constexpr int kBunkerCount = 6;
static constexpr int kPondCount = 4;
inline bool inEllipse(float x, float z, const HazardEllipse &h) {
    const float dx=(x-h.x)/h.rx, dz=(z-h.z)/h.rz;
    return dx*dx + dz*dz < 1.0f;
}

struct Game {
    float holeX = 0.0f;
    // Per-hole target; initially matches the previous single 92m hole.
    float holeZ = 92.0f;
    int holePar = 3;
    int holeIndex = 0;
    float x = 0, y = 0, z = 0;
    float vx = 0, vy = 0, vz = 0;
    float hitSide = 0.0f;  // -1: left, +1: right
    float currentPower = .5f;
    float flightSide = 0;
    // Wind vector points in the direction the air is moving, m/s.
    // It is deliberately a light crosswind in R2; individual holes may vary it.
    float windX = 3.0f, windZ = -1.2f;
    float windSpeed() const { return length2(windX,windZ); }
    float aimX = 0, aimZ = 1;
    // Player-selected aim offset from the direction of the cup, in radians.
    // Negative = left, positive = right; does not alter impact sidespin.
    float aimOffset = 0.0f;
    // ON: heading follows the cup as the ball moves.
    // OFF: retain a fixed world-space heading until the player steers.
    bool autoAimToCup = true;
    float fixedAimBaseX = 0.0f, fixedAimBaseZ = 1.0f;
    float distance = 92;
    float meterClock = 0;
    int shots = 0;
    int bounce = 0;
    // A water hazard adds two penalty strokes after the sinking animation.
    int penaltyStrokes = 0;
    float prevShotX = 0, prevShotZ = 0;
    float waterClock = 0.0f;
    bool ravineSink = false;
    static constexpr float WATER_VIEW_SECONDS = 3.0f;
    State state = State::READY;
    bool lastWasPutt = false;

    const HoleLayout& course() const { return kNineHoles[holeIndex]; }
    int bunkerCount() const { return course().bunkerCount; }
    int pondCount() const { return course().pondCount; }
    int treeCount() const { return course().treeCount; }
    HazardEllipse bunker(int i) const { return course().bunkers[i]; }
    HazardEllipse pond(int i) const { return course().ponds[i]; }
    TreeMark treeAt(int i) const { return course().trees[i]; }
    void fairwayAt(float zAt, float &center, float &halfWidth) const {
        const HoleLayout &c=course();
        if(zAt<=c.path[0].z) {center=c.path[0].x;halfWidth=c.path[0].halfWidth;return;}
        for(int i=1;i<c.fairwayCount;++i) {
            const FairwayNode &a=c.path[i-1], &b=c.path[i];
            if(zAt<=b.z) {
                float t=(zAt-a.z)/(b.z-a.z);
                center=a.x+(b.x-a.x)*t;
                halfWidth=a.halfWidth+(b.halfWidth-a.halfWidth)*t;
                return;
            }
        }
        center=c.path[c.fairwayCount-1].x;
        halfWidth=c.path[c.fairwayCount-1].halfWidth;
    }
    bool onFairway() const {
        float center,width;
        fairwayAt(z,center,width);
        return fabsf(x-center)<=width && z>=-10.0f && z<=holeZ+14.0f;
    }
    bool inRavine() const {
        const HoleLayout &c=course();
        return c.terrain==RAVINE && z>c.ravineStart && z<c.ravineEnd && fabsf(x)<36.0f;
    }
    void setHole(int n) {
        reset();
        if (n < 0) n = 0;
        if (n >= kHoleCount) n = kHoleCount - 1;
        holeIndex = n;
        holeZ = course().distance;
        holeX = course().cupX;
        holePar = course().par;
        distance = length2(holeX,holeZ);
        updateAim();
    }
    void reset() {
        x = y = z = vx = vy = vz = 0;
        hitSide = 0;
        currentPower = .5f;
        flightSide = 0;
        windX = 3.0f; windZ = -1.2f;
        aimX = 0; aimZ = 1;
        aimOffset = 0.0f;
        autoAimToCup = true;
        fixedAimBaseX = 0.0f; fixedAimBaseZ = 1.0f;
        holeIndex = 0;
        holeZ = kNineHoles[0].distance;
        holeX = kNineHoles[0].cupX;
        holePar = kNineHoles[0].par;
        distance = length2(holeX,holeZ);
        meterClock = 0;
        shots = bounce = penaltyStrokes = 0;
        prevShotX = prevShotZ = waterClock = 0.0f;
        ravineSink=false;
        state = State::READY;
        lastWasPutt = false;
    }

    // Deterministic breeze for each new round. Wind is in WORLD X/Z,
    // independent of the camera, and remains fixed throughout this hole.
    // This deliberately avoids depending on Arduino RNG in the physics core.
    void setWindSeed(uint32_t seed) {
        uint32_t s = seed ? seed : 0x9E3779B9u;
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        // Roughly 0.9..4.1 m/s with a broad range of directions.
        float angle = (float)(s & 0xffffu) * (6.28318530718f / 65536.0f);
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        float speed = 0.9f + 3.2f * (float)(s & 0xffffu) / 65535.0f;
        windX = speed * cosf(angle);
        windZ = speed * sinf(angle);
    }

    void updateAim() {
        float dx = holeX-x, dz = holeZ-z;
        distance = length2(dx,dz);
        // Distance always tracks the cup; orientation only does so in AUTO mode.
        if (!autoAimToCup || distance > .01f) {
            const float baseX = autoAimToCup ? dx/distance : fixedAimBaseX;
            const float baseZ = autoAimToCup ? dz/distance : fixedAimBaseZ;
            const float co = cosf(aimOffset), si = sinf(aimOffset);
            aimX = baseX*co + baseZ*si;
            aimZ = baseZ*co - baseX*si;
        }
    }

    void setAutoAimToCup(bool enabled, bool startOfHole = false) {
        if (enabled) {
            // Explicitly choosing AUTO points at the cup immediately.
            autoAimToCup = true;
            aimOffset = 0.0f;
        } else if (autoAimToCup || startOfHole) {
            // Switching OFF mid-hole keeps the currently visible bearing.
            // A NEW hole in MANUAL mode starts looking down the fairway (+Z).
            fixedAimBaseX = startOfHole ? 0.0f : aimX;
            fixedAimBaseZ = startOfHole ? 1.0f : aimZ;
            aimOffset = 0.0f;
            autoAimToCup = false;
        }
        updateAim();
    }

    // Each tap turns 5 degrees. The new heading is used both by the
    // first-person camera and by the actual launch velocity in shoot().
    void steerAim(int dir, int stepDegrees=5) {
        if (state != State::READY || dir == 0) return;
        const float step = clampf((float)stepDegrees,1.0f,10.0f)*0.01745329251994f;
        static constexpr float LIMIT = 1.308996939f;  // +/- 75 degrees
        aimOffset = clampf(aimOffset + (dir < 0 ? -step : step), -LIMIT, LIMIT);
        updateAim();
    }

    bool shoot() {
        if (state != State::READY) return false;
        updateAim();
        // Remember the exact lie before this stroke for the water drop.
        prevShotX=x;
        prevShotZ=z;
        const bool sandLie=inSand();
        bool putt = distance < 27.0f;
        lastWasPutt = putt;
        float rX = aimZ, rZ = -aimX;
        float steer = hitSide * (putt ? .12f : .09f);
        float dx = aimX + steer*rX, dz = aimZ + steer*rZ;
        float norm = length2(dx,dz);
        dx /= norm; dz /= norm;
        if (putt) {
            const float speed = (1.0f + 10.6f * currentPower) * (sandLie ? .68f : 1.0f);
            vx = dx * speed;
            vz = dz * speed;
            vy = 0;
            y = 0;
            state = State::ROLLING;
        } else {
            const float speed = (10.0f + 32.0f * currentPower) * (sandLie ? .68f : 1.0f);
            const float elev = .53f; // 30.4 degrees
            vx = dx * speed * cosf(elev);
            vz = dz * speed * cosf(elev);
            vy = speed * sinf(elev);
            y = .12f;
            bounce = 0;
            state = State::AIRBORNE;
        }
        flightSide = hitSide;
        ++shots;
        return true;
    }

    bool onGreen() const {
        return length2(x-holeX, z-holeZ) < 18.0f;
    }
    bool inSand() const {
        for (int i=0;i<bunkerCount();++i) if (inEllipse(x,z,bunker(i))) return true;
        return false;
    }
    bool inWater() const {
        for (int i=0;i<pondCount();++i) if (inEllipse(x,z,pond(i))) return true;
        return false;
    }

    // Returns a bitmask of immediate sound/vibration events.
    uint8_t step(float dt) {
        dt = clampf(dt, .0f, .04f);
        meterClock += dt;
        float wave = .5f + .5f*sinf(meterClock * 3.9f);
        currentPower = .04f + .96f*wave;
        if (state == State::READY || state == State::HOLED) return NONE;
        if (state == State::SINKING) {
            waterClock += dt;
            if (waterClock >= WATER_VIEW_SECONDS) {
                // The previous shot is already counted by shoot(). Add TWO more.
                shots += 2;
                penaltyStrokes += 2;
                x=prevShotX; z=prevShotZ; y=0;
                vx=vy=vz=0;
                bounce=0;
                state=State::READY;
                waterClock=0.0f;
                ravineSink=false;
                updateAim();
                return WATER_RESET;
            }
            return NONE;
        }

        uint8_t events = NONE;
        if (state == State::AIRBORNE) {
            float hs = length2(vx, vz);
            // Aerodynamic sidespin produces a progressive, gentle hook or slice.
            float rightX = aimZ, rightZ = -aimX;
            float acc = flightSide * 2.15f * clampf(hs/30.0f, .3f, 1.4f);
            vx += rightX * acc * dt;
            vz += rightZ * acc * dt;
            // Very small height-dependent wind response; no wind while rolling.
            // 3.23 m/s wind -> at most ~0.21 m/s^2 before time integration.
            // Near-ground wind effect tends smoothly to zero.
            float altitude = clampf(y / 11.0f, 0.0f, 1.0f);
            constexpr float windResponse = 0.065f;
            vx += windX * windResponse * altitude * dt;
            vz += windZ * windResponse * altitude * dt;
            vy -= 9.81f * dt;
            x += vx * dt;
            z += vz * dt;
            y += vy * dt;
            if (y <= 0.0f) {
                y = 0;
                events |= LANDING;
                if (bounce < 2 && vy < -5.5f && !inSand()) {
                    vy *= -.22f;
                    vx *= .50f;
                    vz *= .50f;
                    ++bounce;
                } else {
                    vy = 0;
                    state = State::ROLLING;
                }
            }
        } else {
            float speed = length2(vx,vz);
            float f = inSand() ? 8.0f : (onGreen() ? 2.1f : (onFairway() ? 3.0f : 4.5f));
            float next = speed - f * dt;
            if (next < 0) next = 0;
            if (speed > .001f) {
                float k = next / speed;
                vx *= k; vz *= k;
                // Small residual side-spin. At low speed it decays naturally.
                float acc = flightSide * .12f * next / 10.0f;
                vx += aimZ * acc * dt;
                vz -= aimX * acc * dt;
            }
            x += vx * dt;
            z += vz * dt;
        }
        // Forest corridors are real obstacles, not painted-only trees.
        // Rebound from a trunk at low altitude, while retaining the normal
        // ground/air simulation and existing shot controls.
        if (y >= 0.0f && y < 5.5f && (fabsf(vx)+fabsf(vz)) > .1f) {
            for(int i=0;i<treeCount();++i) {
                TreeMark t=treeAt(i);
                if(length2(x-t.x,z-t.z)<.75f) {
                    vx *= -.35f; vz *= -.35f;
                    if(state==State::AIRBORNE) vy=fmaxf(vy*.22f,0.0f);
                    events |= TREE_HIT;
                    break;
                }
            }
        }
        updateAim();
        // Ravine is a separate landing hazard on Hole 5, using the same
        // two-stroke return-to-last-lie rule as the existing pond.
        if (y <= .01f && inRavine()) {
            state=State::SINKING;
            ravineSink=true;
            waterClock=0.0f;
            vx=vy=vz=0;
            return events | RAVINE_HIT;
        }
        // A shot enters the pond only once the ball reaches the surface.
        if (y <= 0.01f && inWater()) {
            state=State::SINKING;
            ravineSink=false;
            waterClock=0.0f;
            vx=vy=vz=0;
            return events | WATER_HIT;
        }
        float speed = length2(vx,vz);
        if (y < .15f && distance < .85f && speed < 7.0f) {
            x = holeX; z = holeZ; y = -.3f;
            vx = vy = vz = 0;
            state = State::HOLED;
            return events | CUP;
        }
        if (state == State::ROLLING && speed < .18f) {
            vx=vy=vz=0;
            state = State::READY;
            events |= STOP;
        }
        return events;
    }
};

}  // namespace beg
