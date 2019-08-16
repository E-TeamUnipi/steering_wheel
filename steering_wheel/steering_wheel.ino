#include "byteorder.h"
#include "display.h"
#include "can_handler.h"
#include "avdweb_Switch.h"

// Remove loop overhead, never called!
void loop(){}

void setup()
{
    Serial.begin(115200);

    while(!can_handler::init());

    display::init();

    _main();
}

Switch upshift{7};
Switch downshift{6};
Switch neutral{12};
Switch limiter{11};
Switch launch{10};
Switch next_layout{9};


CAN_FRAME can_500{0};

// The neutral is not saved directly inside the frame because we want that
// works only on paddle shifts
uint16_t neutral_field{0};

bool limiter_on{false};

bool upshift_possible()
{
    return can_handler::hz3.GEAR < 4 || can_handler::hz3.GEAR >= 8;
}

void shift_release(void* t_arg)
{
    can_500.data.s0 = byteorder::htocs(0x029A); // shift
    can_500.data.s1 = 0x0000;                   // neutral

    can_handler::send(can_500);
}

void upshift_push(void* t_arg)
{
    if (!upshift_possible()) {
        return;
    }

    can_500.data.s0 = byteorder::htocs(0x006f);
    can_500.data.s1 = neutral_field;

    can_handler::send(can_500);
}

void downshift_push(void * t_arg)
{
    can_500.data.s0 = byteorder::htocs(0x03e7);
    can_500.data.s1 = neutral_field;

    can_handler::send(can_500);
}

void neutral_longpress(void* t_arg)
{
    neutral_field = byteorder::htocs(0x03ff);
//    can_500.data.s1 = byteorder::htocs(0x03ff);

//    can_handler::send(can_500);
}

void neutral_release(void * t_arg)
{
    neutral_field = 0x0000;
//    can_500.data.s1 = 0x0000;

//    can_handler::send(can_500);
}

void limiter_push(void * t_arg)
{
    can_500.data.s3 = limiter_on? 0x0000: byteorder::htocs(500);
    limiter_on = !limiter_on;

    can_handler::send(can_500);
}

void limiter_release(void * t_arg)
{
//    can_500.data.s3 = 0x0000;
//
//    can_handler::send(can_500);
}

void launch_push(void * t_arg)
{
    can_500.data.s1 = 0x0000; // neutral off
    can_500.data.s2 = 0x0000; // launch

    can_handler::send(can_500);
}

void launch_release(void* t_arg)
{
    can_500.data.s2 = byteorder::htocs(0x3ff);  // launch

    can_handler::send(can_500);
}

void next_layout_push(void * t_arg)
{
    can_handler::hz3.NEXT_LAYOUT = 1;
}

void _main()
{
    can_500.length = sizeof(CAN_FRAME);
    can_500.id = 0x500;
    can_500.data.s0 = byteorder::htocs(0x029A); // shift
    can_500.data.s1 = 0x0000;                   // neutral
    can_500.data.s2 = byteorder::htocs(0x3ff);  // launch
    can_500.data.s3 = 0x0000;                   // limiter
    
    upshift.setPushedCallback(&upshift_push, nullptr);
    upshift.setReleasedCallback(&shift_release, nullptr);
    
    downshift.setPushedCallback(&downshift_push, nullptr);
    downshift.setReleasedCallback(&shift_release, nullptr);

    neutral.setLongPressCallback(&neutral_longpress, nullptr);
    neutral.setReleasedCallback(&neutral_release, nullptr);

    limiter.setPushedCallback(&limiter_push, nullptr);
    limiter.setReleasedCallback(&limiter_release, nullptr);
    
    
    launch.setPushedCallback(&launch_push, nullptr);
    launch.setReleasedCallback(&launch_release, nullptr);
    
    next_layout.setPushedCallback(&next_layout_push, nullptr);

    uint64_t launch_time{0};
    uint64_t last_launch_shift{0};
    uint64_t curr_time{0};
    
    for(;;) {
        curr_time = millis();

        neutral.poll();
        upshift.poll();
        downshift.poll();
        limiter.poll();
        launch.poll();
        next_layout.poll();


        // ( LAUNCH CONTROL
        launch_time = can_handler::hz3.LAUNCH? curr_time: launch_time;
        if (curr_time - launch_time < 10000 && curr_time - last_launch_shift > 200 /*&& can_handler::hz3.LAUNCH*/) {
            bool shift_flag{false};
                
            switch (can_handler::hz3.GEAR) {
                case 0: {
                    break;
                }
                case 1:
                case 2:
                case 3: {
                    if (can_handler::hz25.RPM >= 12000) {
                        upshift_push(nullptr);

                        last_launch_shift = curr_time;
                        shift_flag = true;
                    }
                        
                    break;
                }
            }

            if (shift_flag) {
                // efi rest state after shift
                delay(40);
                shift_release(nullptr);
            }
                

        }
        // )

        display::update();
    }
}


