#ifndef can_handler_h
#define can_handler_h

#include <utility>
#include <due_can.h>
#include "byteorder.h"

namespace can_handler {

    struct __attribute__((packed)) {
        uint8_t sep[4]{'A', 'C', 'K', 25};
        uint16_t RPM{0};
        float TPS{0};
        float PPS{0};
        float MAP{0};
    } volatile hz25;

    struct __attribute__((packed)) {
        uint8_t sep[4]{'A', 'C', 'K', 3};
        float EOT{25.4}; // engine oil temp
        float ECT{20.2}; // engine coolant(water) temp
        float EOP{20}; // engine oil pressure

        float BATTERY{12.3};

        uint8_t GEAR{1};

        uint8_t LIMP{0};
        uint8_t LAUNCH{0};

        uint8_t NEXT_LAYOUT{0};
    } volatile hz3;
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
                break;
            }
            case 0x302: {
                can_handler::hz3.GEAR = t_frame->data.bytes[5];
                can_handler::hz3.LIMP = t_frame->data.s1;
                can_handler::hz25.PPS = map_float(byteorder::ctohs(t_frame->data.s3), 0x000, 0x3ff, 0, 100);
                break;
            }
            case 0x304: {
                can_handler::hz25.RPM = byteorder::ctohs(t_frame->data.s0);
                can_handler::hz3.LAUNCH = byteorder::ctohs(t_frame->data.s1);
                can_handler::hz3.ECT = map_float(byteorder::ctohs(t_frame->data.s3), 0x20, 0xff, 10, 150);
                can_handler::hz25.TPS = map_float(byteorder::ctohs(t_frame->data.s2), 0x000, 0x3ff, 0, 100);
                break;
            }
            case 0x306: {
                can_handler::hz3.EOP = byteorder::ctohs(t_frame->data.s0);
                can_handler::hz3.EOT = map_float(byteorder::ctohs(t_frame->data.s1), 0x20, 0xff, 10, 150);
                break;
            }
            case 0x308: {
                can_handler::hz3.BATTERY = t_frame->data.bytes[5] * 14.25 / 255.0;
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

