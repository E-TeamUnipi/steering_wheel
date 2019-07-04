#ifndef display_h
#define display_h

#include <DueTimer.h>

// to get directly access to can structures
#include "can_handler.h"

// hidden
namespace {
    struct {
        bool hertz_25{false};
        bool hertz_3{false};  
    } volatile clocks;
}

namespace display {
    inline
    void update()
    {
        if (clocks.hertz_25) {
            clocks.hertz_25 = false;
            Serial.write(
                (uint8_t *) &can_handler::hz25,
                sizeof(can_handler::hz25)
            );
        }

        if (clocks.hertz_3) {
            clocks.hertz_3 = false;
            Serial.write(
                (uint8_t *) &can_handler::hz3,
                sizeof(can_handler::hz3)
            );
            can_handler::hz3.NEXT_LAYOUT = 0;
        }
    }

    void init()
    {
        Timer0.attachInterrupt([]() {
            clocks.hertz_25 = true;
        }).setFrequency(25).start();

        Timer1.attachInterrupt([]() {
            clocks.hertz_3 = true;
        }).setFrequency(3).start();
    }
}

#endif

