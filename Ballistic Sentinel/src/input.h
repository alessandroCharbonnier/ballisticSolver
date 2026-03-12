#pragma once
/// @file input.h
/// @brief 5-way navigation button handler with debounce and long-press.

#include <Arduino.h>
#include "config.h"

enum class ButtonId : uint8_t {
    NONE, UP, DOWN, LEFT, RIGHT, CENTER
};

enum class ButtonEvent : uint8_t {
    NONE,
    PRESS,        // normal short press
    LONG_PRESS,   // held for BTN_LONG_PRESS_MS
    REPEAT        // auto-repeat while held
};

struct ButtonState {
    ButtonId    id;
    ButtonEvent event;
    ButtonState(ButtonId i = ButtonId::NONE, ButtonEvent e = ButtonEvent::NONE)
        : id(i), event(e) {}
};

class Input {
public:
    void begin();

    /// Call every loop iteration.  Returns the most recent event (if any).
    ButtonState poll();

private:
    struct BtnInfo {
        uint8_t      pin;
        ButtonId     id;
        bool         pressed;
        uint32_t     pressed_at;
        uint32_t     last_repeat;
        bool         long_fired;
        BtnInfo(uint8_t p, ButtonId i)
            : pin(p), id(i), pressed(false), pressed_at(0),
              last_repeat(0), long_fired(false) {}
    };

    BtnInfo btns_[5] = {
        { cfg::PIN_BTN_UP,     ButtonId::UP     },
        { cfg::PIN_BTN_DOWN,   ButtonId::DOWN   },
        { cfg::PIN_BTN_LEFT,   ButtonId::LEFT   },
        { cfg::PIN_BTN_RIGHT,  ButtonId::RIGHT  },
        { cfg::PIN_BTN_CENTER, ButtonId::CENTER  },
    };

    uint32_t last_debounce_ = 0;
};
