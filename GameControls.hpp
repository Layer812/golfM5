#pragma once
// BALL'S EYE GOLF - pure game touch hit testing (466-unit design coords).
// A gesture captures a control at touch-down. Dragging through the HIT area
// never fires a shot: only the physical right button (BtnB) does.
#include "GamePhysics.hpp"
namespace beg {
enum class TouchTarget : uint8_t { NONE, IMPACT, FIRE, REPLAY, AIM_LEFT, AIM_RIGHT };
inline TouchTarget hitTest(float x, float y, bool holed) {
    // Reject touches outside the circular display, including the corners.
    float dx=x-233.f, dy=y-233.f;
    if (dx*dx+dy*dy > 232.f*232.f) return TouchTarget::NONE;
    if (holed) return TouchTarget::REPLAY;
    // Large, circular-screen-safe arrow hit areas beside the ball-eye view.
    if (y >= 201.f && y <= 275.f) {
        if (x >= 21.f && x <= 96.f) return TouchTarget::AIM_LEFT;
        if (x >= 370.f && x <= 445.f) return TouchTarget::AIM_RIGHT;
    }
    if (y >= 329.f && y <= 432.f) {
        if (x >= 115.f && x <= 223.f) return TouchTarget::IMPACT;
        // POWER meter is display-only: physical right button fires the ball.
    }
    return TouchTarget::NONE;
}
inline float hitSideAt(float x) {
    return clampf((x-165.f)/26.f, -1.f, 1.f);
}
struct TouchGesture {
    TouchTarget captured = TouchTarget::NONE;
    bool wasPressed = false;
    // Call once each M5.update(), with screen coords already normalized.
    // Returns REPLAY/aim actions only on *new* touch-down.
    TouchTarget update(bool pressed, float x, float y, bool holed,
                       float &hitSide) {
        if (!pressed) {
            captured = TouchTarget::NONE;
            wasPressed = false;
            return TouchTarget::NONE;
        }
        if (!wasPressed) {
            wasPressed = true;
            captured = hitTest(x,y,holed);
            if (captured == TouchTarget::IMPACT) {
                hitSide = hitSideAt(x);
                return TouchTarget::IMPACT;
            }
            return captured;
        }
        if (captured == TouchTarget::IMPACT && !holed) {
            hitSide = hitSideAt(x);
            return TouchTarget::IMPACT;
        }
        return TouchTarget::NONE;
    }
    void reset() { captured=TouchTarget::NONE; wasPressed=false; }
};
}  // namespace beg
