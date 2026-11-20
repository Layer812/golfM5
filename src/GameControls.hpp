// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Layer812
#pragma once
// BALL'S EYE GOLF - pure game touch hit testing (466-unit design coords).
// A gesture captures a control at touch-down:
// - 6 o'clock IMPACT pad adjusts horizontal hit point while held.
// - Dragging anywhere else on the circular screen pans the aim heading.
// Only the physical right button (BtnB) fires a shot.
#include "GamePhysics.hpp"
namespace beg {
enum class TouchTarget : uint8_t { NONE, IMPACT, FIRE, REPLAY, AIM_DRAG };
inline TouchTarget hitTest(float x, float y, bool holed) {
    // Reject touches outside the circular display, including the corners.
    float dx=x-233.f, dy=y-233.f;
    if (dx*dx+dy*dy > 232.f*232.f) return TouchTarget::NONE;
    if (holed) return TouchTarget::REPLAY;
    // Bottom-center (6 o'clock) IMPACT pad.
    if (y >= 338.f && y <= 454.f && x >= 184.f && x <= 282.f) {
        return TouchTarget::IMPACT;
    }
    // Anywhere else inside the round glass initiates horizontal swipe aim.
    return TouchTarget::AIM_DRAG;
}
inline float hitSideAt(float x) {
    return clampf((x-233.f)/28.f, -1.f, 1.f);
}
struct TouchGesture {
    TouchTarget captured = TouchTarget::NONE;
    bool wasPressed = false;
    float lastX = 0.f;
    float aimDragDeltaPx = 0.f;
    bool isEditingImpact() const {
        return wasPressed && captured == TouchTarget::IMPACT;
    }
    // Call once each M5.update(), with screen coords already normalized.
    // Returns REPLAY on new touch-down, IMPACT while dragging the impact pad,
    // and AIM_DRAG when horizontal swipe motion occurs.
    TouchTarget update(bool pressed, float x, float y, bool holed,
                       float &hitSide) {
        aimDragDeltaPx = 0.f;
        if (!pressed) {
            captured = TouchTarget::NONE;
            wasPressed = false;
            return TouchTarget::NONE;
        }
        if (!wasPressed) {
            wasPressed = true;
            captured = hitTest(x,y,holed);
            lastX = x;
            if (captured == TouchTarget::IMPACT) {
                hitSide = hitSideAt(x);
                return TouchTarget::IMPACT;
            }
            if (captured == TouchTarget::REPLAY) {
                return TouchTarget::REPLAY;
            }
            return TouchTarget::NONE;
        }
        if (captured == TouchTarget::IMPACT && !holed) {
            hitSide = hitSideAt(x);
            return TouchTarget::IMPACT;
        }
        if (captured == TouchTarget::AIM_DRAG && !holed) {
            const float dx = x - lastX;
            if (fabsf(dx) >= 1.5f) {
                aimDragDeltaPx = dx;
                lastX = x;
                return TouchTarget::AIM_DRAG;
            }
        }
        return TouchTarget::NONE;
    }
    void reset() {
        captured=TouchTarget::NONE;
        wasPressed=false;
        lastX=0.f;
        aimDragDeltaPx=0.f;
    }
};
}  // namespace beg
