#include <DueTimer.h>
#include <RingBuf.h>

#include "byteorder.h"
#include "display.h"
#include "can_handler.h"
#include "click_handler.h"

// Remove loop overhead, never called!
void loop(){}

void setup()
{
    Serial.begin(115200);

    click_handler::init();
    Serial.println("I'm ready!");

    //Serial.println(htonl(0xff00ff00), HEX);

    // test canbus byteorder
    Serial.print("long \t0x");
    Serial.print(byteorder::ctohl(0xff00ff00), HEX);
    Serial.print(" \t0x");
    Serial.print(0x00ff00ff, HEX);
    if (byteorder::ctohl(0xff00ff00) == 0x00ff00ff) {
        Serial.println(" \tPASSED!");
    } else {
        Serial.println(" \tFAILED!");
    }

    Serial.print("short \t0x");
    Serial.print(byteorder::ctohs(0xff00), HEX);
    Serial.print(" \t\t0x");
    Serial.print(0x00ff, HEX);
    if (byteorder::ctohs(0xff00) == 0x00ff) {
        Serial.println(" \t\tPASSED!");
    } else {
        Serial.println(" \t\tFAILED!");
    }

    // Test to send data with interrupts
    Timer0.attachInterrupt([](){
        static uint8_t msg_id{0xf0};
        static uint16_t msg{0};

        CAN_FRAME outgoing;
        outgoing.id = msg_id;
        outgoing.data.s0 = msg;

        ++msg;
        msg_id = (msg_id ==0x40)? 0xf0:0x40;
        
        can_handler::m_ring.push(outgoing);
    }).setFrequency(2000).start();

    _main();
}

void _main()
{
    uint8_t count{0};
    for(;;) {
        uint8_t data;
        CAN_FRAME frame;
        if (click_handler::m_ring.lockedPop(data)) {
            Serial.print(data);
            Serial.print(" clicks " );
            Serial.println(++count);

            CAN_FRAME outgoing;
            outgoing.id = 0x400;
            outgoing.extended = false;
            outgoing.priority = 4; //0-15 lower is higher priority
            outgoing.data.byte[2] = 0xDD;

            outgoing.data.s0 = 0xFEED;
            outgoing.data.byte[3] = 0x55;
            outgoing.data.high = 0xDEADBEEF;
            can_handler::send(outgoing);
        }

        if (can_handler::m_ring.lockedPop(frame)) {
//            Serial.print(" ID: 0x");
//            Serial.print(frame.id, HEX);
//            Serial.print(" Len: ");
//            Serial.print(frame.length);
//            Serial.print(" Data: 0x");
//            for (int count = 0; count < frame.length; count++) {
//                Serial.print(frame.data.bytes[count], HEX);
//                Serial.print(" ");
//            }
//            Serial.print("\r\n");
//              display::send(frame.id, frame.data.s0);
    
        }



    }
}


