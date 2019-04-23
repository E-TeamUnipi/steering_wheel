#include "byteorder.h"
#include "display.h"
#include "can_handler.h"

#define SOFTWARE_DEBOUNCING 1

#include "click_handler.h"

// Remove loop overhead, never called!
void loop(){}

void setup()
{
    Serial.begin(115200);

    while(!can_handler::init());
    click_handler::init();
    display::init();

    _main();
}

void _main()
{
    uint8_t count{0};

    for(;;) {
        uint8_t pin;
        CAN_FRAME frame;
        if (click_handler::m_ring.lockedPop(pin)) {
            CAN_FRAME outgoing;
            outgoing.id = 0x001;
            outgoing.extended = false;
            outgoing.priority = 4;
            outgoing.length = 1;
            outgoing.data.s0 = count;
        }

        display::update();
    }
}


