#include "input.h"
#include <esp_sleep.h>

// Volatile ISR flag — set on any button state change
static volatile bool s_button_change = false;

static void IRAM_ATTR buttonISR() {
    s_button_change = true;
}

void Input::begin() {
    for (auto& b : btns_) {
        pinMode(b.pin, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(b.pin), buttonISR, CHANGE);
    }
}

void Input::configureWakeup() {
    esp_sleep_enable_ext0_wakeup(
        static_cast<gpio_num_t>(cfg::PIN_BTN_CENTER), 0);  // wake on LOW
}

ButtonState Input::poll() {
    uint32_t now = millis();

    // Check CENTER double-press timeout → emit deferred single PRESS
    if (center_waiting_
        && (now - center_first_release_ > cfg::BTN_DOUBLE_PRESS_MS))
    {
        center_waiting_ = false;
        center_press_count_ = 0;
        return {ButtonId::CENTER, ButtonEvent::PRESS};
    }

    // Debounce guard
    if (now - last_debounce_ < cfg::BTN_DEBOUNCE_MS) {
        return {ButtonId::NONE, ButtonEvent::NONE};
    }
    last_debounce_ = now;

    ButtonState result = {ButtonId::NONE, ButtonEvent::NONE};

    for (auto& b : btns_) {
        bool raw = (digitalRead(b.pin) == LOW);  // active-low

        if (raw && !b.pressed) {
            // ── Just pressed ──
            b.pressed     = true;
            b.pressed_at  = now;
            b.last_repeat = now;
            b.long_fired  = false;

            if (b.id == ButtonId::CENTER) {
                center_press_count_++;
            } else {
                result = {b.id, ButtonEvent::PRESS};
            }

        } else if (raw && b.pressed) {
            // ── Held ──
            uint32_t held = now - b.pressed_at;

            if (b.id == ButtonId::CENTER) {
                // 5-second hold → LONG_PRESS (deep sleep trigger)
                if (!b.long_fired && held >= cfg::BTN_LONG_PRESS_MS) {
                    b.long_fired = true;
                    center_press_count_ = 0;
                    center_waiting_ = false;
                    result = {ButtonId::CENTER, ButtonEvent::LONG_PRESS};
                }
            } else {
                // Directional auto-repeat
                if (held >= cfg::BTN_REPEAT_DELAY_MS
                    && (now - b.last_repeat) >= cfg::BTN_REPEAT_RATE_MS)
                {
                    b.last_repeat = now;
                    result = {b.id, ButtonEvent::REPEAT};
                }
            }

        } else if (!raw && b.pressed) {
            // ── Released ──
            b.pressed     = false;
            b.released_at = now;

            if (b.id == ButtonId::CENTER && !b.long_fired) {
                if (center_press_count_ >= 2) {
                    // Second release → double press
                    center_press_count_ = 0;
                    center_waiting_ = false;
                    result = {ButtonId::CENTER, ButtonEvent::DOUBLE_PRESS};
                } else {
                    // First release → wait for possible second press
                    center_first_release_ = now;
                    center_waiting_ = true;
                }
            }
        }
    }

    s_button_change = false;
    return result;
}
