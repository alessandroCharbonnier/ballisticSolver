#include "input.h"

void Input::begin() {
    for (auto& b : btns_) {
        pinMode(b.pin, INPUT_PULLUP);  // active-low
    }
}

ButtonState Input::poll() {
    uint32_t now = millis();

    // Debounce guard
    if (now - last_debounce_ < cfg::BTN_DEBOUNCE_MS) {
        return {ButtonId::NONE, ButtonEvent::NONE};
    }
    last_debounce_ = now;

    ButtonState result = {ButtonId::NONE, ButtonEvent::NONE};

    for (auto& b : btns_) {
        bool raw = (digitalRead(b.pin) == LOW);  // active-low

        if (raw && !b.pressed) {
            // --- Just pressed ---
            b.pressed     = true;
            b.pressed_at  = now;
            b.last_repeat = now;
            b.long_fired  = false;
            result = {b.id, ButtonEvent::PRESS};

        } else if (raw && b.pressed) {
            // --- Held ---
            uint32_t held = now - b.pressed_at;

            // Long press detection (center button only)
            if (!b.long_fired && b.id == ButtonId::CENTER
                && held >= cfg::BTN_LONG_PRESS_MS)
            {
                b.long_fired = true;
                result = {b.id, ButtonEvent::LONG_PRESS};
            }
            // Auto-repeat (directional buttons)
            else if (b.id != ButtonId::CENTER
                     && held >= cfg::BTN_REPEAT_DELAY_MS
                     && (now - b.last_repeat) >= cfg::BTN_REPEAT_RATE_MS)
            {
                b.last_repeat = now;
                result = {b.id, ButtonEvent::REPEAT};
            }

        } else if (!raw && b.pressed) {
            // --- Released ---
            b.pressed = false;
        }
    }

    return result;
}
