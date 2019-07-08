#include "byteorder.h"
#include "display.h"
#include "can_handler.h"

// #define SOFTWARE_DEBOUNCING 1
// #include "click_handler.h"

// Remove loop overhead, never called!
void loop(){}

struct {
    bool upshift{false};
    bool downshift{false};  
    bool neutral{false};
    bool launch{false};
    bool next_layout{false};
} volatile buttons_status;

    
static constexpr struct {
    const uint8_t upshift{7};
    const uint8_t downshift{6};
    const uint8_t neutral{5};
    const uint8_t launch{4};
        
    const uint8_t next_layout{2};
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

void _main()
{
    uint64_t last_cmd = 0;
    uint64_t last_rest = 0;
    uint64_t curr_time = millis();
    volatile bool neutral_enabled = false;
    
    for(;;) {
        update_buttons_status();
        bool shifted{true};

        curr_time = millis();

        if (curr_time - last_cmd > 100) {
            last_cmd = ([&last_rest, curr_time, &shifted]() {
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

                
                if (can_handler::hz3.LAUNCH && can_handler::hz3.GEAR < 4) {
                    if (can_handler::hz25.RPM > 10900 && can_handler::car_speed >= 5) {
                        can_shift.data.s0 = byteorder::htocs(0x006f);
                        can_shift.data.s1 = 0x0000;
                        can_handler::send(can_shift);
                        return true;
                    }    
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
                    can_shift.data.s2 = can_handler::hz3.LAUNCH? 0: byteorder::htocs(1);
                    can_handler::send(can_shift);

                    return true;
                }

                
                return false;
            }())? curr_time: last_cmd;
        }

    
        display::update();
    }
}


