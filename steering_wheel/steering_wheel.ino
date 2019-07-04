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
    volatile bool neutral_enabled = false;
    
    for(;;) {
        click_handler::pin_click p;
        CAN_FRAME frame;
        if (click_handler::m_ring.lockedPop(p)) {
            
            if (p.pin == click_handler::pins.next_layout && p.type == click_handler::click_type::DOWN) {
                can_handler::hz3.NEXT_LAYOUT = 1;
            }


            if (p.pin == click_handler::pins.neutral) {
                neutral_enabled = p.type == click_handler::click_type::DOWN;

                //can_shift.data.s0 = byteorder::htocs(0x001);
                //can_shift.data.s1 = byteorder::htocs(0x03ff);
                //can_handler::send(can_shift);
            }

            if (p.pin == click_handler::pins.upshift && can_handler::hz3.GEAR >= 4) {
                continue;
            }

            CAN_FRAME can_shift{0};
            can_shift.length = sizeof(CAN_FRAME);
            can_shift.id = 0x500;


            if (p.pin == click_handler::pins.upshift || p.pin == click_handler::pins.downshift) {
                if (p.type == click_handler::click_type::DOWN) {
//                    Serial.print("neutral ");
//                    Serial.println(neutral_enabled);
                    can_shift.data.s0 = p.pin == click_handler::pins.upshift? byteorder::htocs(0x001): byteorder::htocs(0x038f);
                    can_shift.data.s1 = neutral_enabled? byteorder::htocs(0x03ff): 0x0000;
                    can_handler::send(can_shift);
                } else {
                    can_shift.data.s0 = byteorder::htocs(0x028a);
                    can_shift.data.s1 = 0x0000;
                    can_handler::send(can_shift);
                }
            }

            if (p.pin == click_handler::pins.launch && click_handler::click_type::DOWN) {
                can_shift.data.s0 = byteorder::htocs(0x028a);
                can_shift.data.s1 = 0x0000;
                can_handler::hz3.LAUNCH = !can_handler::hz3.LAUNCH;
                can_shift.data.s2 = can_handler::hz3.LAUNCH? byteorder::htocs(1):0;
                can_handler::send(can_shift);
            }
        }

        display::update();
    }
}


