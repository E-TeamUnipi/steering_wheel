#include "byteorder.h"
#include "display.h"
#include "can_handler.h"

// Remove loop overhead, never called!
void loop(){}

struct __attribute__((packed)) {
    bool upshift{false};
    bool downshift{false};
    bool neutral{false};
    bool launch{false};

    bool next_layout{false};
} volatile buttons_status;

    
static constexpr struct __attribute__((packed)) {
    const uint8_t upshift{7};
    const uint8_t downshift{6};
    const uint8_t neutral{2};
    const uint8_t launch{4};
        
    const uint8_t next_layout{22};
} buttons_pins;

void setup()
{
    Serial.begin(115200);

    while(!can_handler::init());

    for (size_t i{0};i < sizeof(buttons_status); ++i) {
        constexpr uint8_t *p{(uint8_t *)&buttons_pins};
        pinMode(p[i], INPUT_PULLUP);
    }

    display::init();

    _main();
}

inline
void update_buttons_status() {
    constexpr uint8_t *bp{(uint8_t *)&buttons_pins};
    constexpr uint8_t *bs{(uint8_t *)&buttons_status};
    for (size_t i{0}; i < sizeof(buttons_status); ++i) {
        bs[i] = digitalRead(bp[i]) == LOW;
    }
}

bool shifted{true};
bool launched{false};
bool layout_changing{false};
uint64_t last_rest{0};

void _main()
{
    uint64_t last_cmd{0};
    uint64_t curr_time = millis();
    
    for(;;) {
        update_buttons_status();
        curr_time = millis();

        if (layout_changing) {
            layout_changing = buttons_status.next_layout;
        } else if (buttons_status.next_layout) {
            layout_changing = true;
            can_handler::hz3.NEXT_LAYOUT = 1; 
        }      

        if (curr_time - last_cmd > 50) {
            last_cmd = ([curr_time]() mutable {
                CAN_FRAME can_shift{0};
                can_shift.length = sizeof(CAN_FRAME);
                can_shift.id = 0x500;
                can_shift.data.s2 = byteorder::htocs(0x3ff);

                if (shifted && !buttons_status.upshift && !buttons_status.downshift) {
                    last_rest = curr_time;
                    // Send rest state to EFI
                    can_shift.data.s0 = byteorder::htocs(0x029A);
                    can_shift.data.s1 = 0x0000;
                    can_handler::send(can_shift);

                    shifted = false;
                }

                if (launched && !buttons_status.launch) {
                    last_rest = curr_time;
                    // Send rest state to EFI
                    can_shift.data.s0 = byteorder::htocs(0x029A);
                    can_shift.data.s1 = 0x0000;
                    can_handler::send(can_shift);

                    launched = false;
                }

                if (can_handler::hz3.LAUNCH && can_handler::hz3.GEAR < 4 &&
                    can_handler::hz25.RPM > 10900 && can_handler::car_speed > 5) {
 
                    can_shift.data.s0 = byteorder::htocs(0x006f);
                    can_shift.data.s1 = 0x0000;
                    can_handler::send(can_shift);

                    return true;
                }
                
                if (buttons_status.upshift && can_handler::hz3.GEAR < 4) {
                    can_shift.data.s0 = byteorder::htocs(0x006f);
                    can_shift.data.s1 = buttons_status.neutral ? byteorder::htocs(0x03ff): 0x0000;
                    can_handler::send(can_shift);

                    shifted = true;
                    return true;
                }
                
                if (buttons_status.downshift) {
                    can_shift.data.s0 = byteorder::htocs(0x03e7);
                    can_shift.data.s1 = buttons_status.neutral ? byteorder::htocs(0x03ff): 0x0000;
                    can_handler::send(can_shift);

                    shifted = true;

                    return true;           
                }

                if (buttons_status.launch) {
                    can_shift.data.s0 = byteorder::htocs(0x029A);
                    can_shift.data.s1 = 0x0000;
                    can_shift.data.s2 = 0x0000;
                    can_handler::send(can_shift);

                    launched = true;

                    return true;
                }
                
                return false;
            }())? curr_time: last_cmd;
        }

    
        display::update();
    }
}


