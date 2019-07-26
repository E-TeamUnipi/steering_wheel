#ifndef can_handler_h
#define can_handler_h

#include <utility>
#include <due_can.h>
#include "byteorder.h"

namespace can_handler {

    struct __attribute__((packed)) {
        uint8_t sep[6]{'K', 'e', 'r', 25, 'u', 'B'};
        uint16_t RPM{0};
        float TPS{0};
        float PPS{0};
        float MAP{0};
    } volatile hz25;

    struct __attribute__((packed)) {
        uint8_t sep[6]{'K', 'e', 'r', 3, 'u', 'B'};
        float EOT{25}; // engine oil temp
        float ECT{20}; // engine coolant(water) temp
        float EOP{20}; // engine oil pressure

        float BATTERY{13.3};

        uint8_t GEAR{4};

        uint8_t LIMP{1};
        uint8_t LAUNCH{0};

        uint8_t NEXT_LAYOUT{0};
    } volatile hz3;

    volatile uint16_t car_speed{0};
}

// hidden
namespace {    
    CANRaw *g_can{nullptr};

    float map_float(float t_val, float t_in_min, float t_in_max, float t_out_min, float t_out_max)
    {
        return (t_val - t_in_min)*(t_out_max - t_out_min)/(t_in_max - t_in_min) + t_out_min;
    }

    void routine(CAN_FRAME *t_frame)
    {
        switch(t_frame->id) {
            case 0x300: {
                //can_handler::hz3.LAUNCH = t_frame->data.bytes[5];
                break;
             }
            case 0x302: {
                can_handler::hz3.GEAR = t_frame->data.bytes[5];
                can_handler::hz3.LIMP = t_frame->data.s1 != 0x0000;
                can_handler::hz25.PPS = map_float(byteorder::ctohs(t_frame->data.s3), 0x000, 0x3ff, 0, 100);
                break;
            }
            case 0x304: {
                can_handler::hz25.RPM = byteorder::ctohs(t_frame->data.s0);
                can_handler::hz3.LAUNCH = byteorder::ctohs(t_frame->data.s1);
                can_handler::hz3.ECT = map_float(byteorder::ctohs(t_frame->data.s3), 0x20, 0xff, 10, 150);
                can_handler::hz25.TPS = map_float(byteorder::ctohs(t_frame->data.s2), 0x00, 0xff, 0, 100);
                break;
            }
            case 0x306: {
                can_handler::hz3.EOP = byteorder::ctohs(t_frame->data.s0);
                can_handler::hz3.EOT = map_float(byteorder::ctohs(t_frame->data.s1), 0x20, 0xff, 10, 150);
                break;
            }
            case 0x308: {

                // 02 FA, FB 13.36, 13.38
                // 02 F3            13.27
                // 02 F1            13.15
                // 13.38:02FB = 13.15:0x2f1
                //13.08, 02ec
                //can_handler::hz3.BATTERY = t_frame->data.bytes[5] * 14.25 / 255.0;
//                can_handler::hz3.BATTERY = map_float(byteorder::ctohs(t_frame->data.s2), 0x2ec, 0x2fa, 13.06, 13.38);
                can_handler::hz3.BATTERY = map_float(byteorder::ctohs(t_frame->data.s2), 0x0000, 0x03ff, 0, 18);
                can_handler::car_speed = byteorder::ctohs(t_frame->data.s3);
                break;
            }
        }
    }
}

namespace can_handler {
    bool init()
    {
        g_can = []() -> CANRaw * {
            if (Can0.init(CAN_BPS_1000K)) {
                return &Can0;
            } else if (Can1.init(CAN_BPS_1000K)) {
                return &Can1;
            }
            return nullptr;
        }();

        if (g_can) {
            // from 0x300 to 0x30F accepted.
            g_can->watchFor(0x300, 0x7F0);
            g_can->setGeneralCallback(routine);

            return true;
        }
        
        return false;
    }

    inline
    void send(CAN_FRAME &t_frame)
    {
        if (g_can) {
            g_can->sendFrame(t_frame);
        }
    }

}

#endif

