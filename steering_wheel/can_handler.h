#ifndef can_handler_h
#define can_handler_h

#include <due_can.h>
#include "byteorder.h"

#define ID_GEAR 0x601

namespace {
    struct can_id_pos {
        uint16_t id;
        uint8_t pos;
    };
}


namespace can_handler {

    static constexpr struct {
        const can_id_pos RPM{0x600, 1};
        const can_id_pos GEAR{0x601, 1};
    } ids;

    struct __attribute__((packed)) {
        uint8_t sep[4]{'A', 'C', 'K', 25};
        uint16_t RPM{0};
        uint16_t TPS{0};
        uint16_t MAP{0};
    } volatile hz25;

    struct __attribute__((packed)) {
        uint8_t sep[4]{'A', 'C', 'K', 3};
        uint16_t EOT{20}; // engine oil temp
        uint16_t ECT{20}; // engine coolant(water) temp
        uint16_t EOP{20}; // engine oil pressure

        uint16_t BATTERY{0};

        uint8_t GEAR{1};

        uint8_t LIMP;
        uint8_t LAUNCH{0};
    } volatile hz3;
}

// hidden
namespace {
    CANRaw *g_can{nullptr};
    constexpr uint32_t can_lst_id{0x600};
    constexpr uint32_t can_hst_id{0x640};

    // Here we receive frames, we filter fast and add
    // it to the ring
    void routine(CAN_FRAME *t_frame)
    {
        // TODO: This is an example to remember
        //       the logic.
        switch(t_frame->id) {
            case can_handler::ids.RPM.id: {
                can_handler::hz25.RPM = byteorder::ctohs(t_frame->data.s0);
                break;
            }
            case ID_GEAR: {
                can_handler::hz3.GEAR = t_frame->data.bytes[0];
                break;
            }
            case 0x603: {
                can_handler::hz3.EOT = byteorder::ctohs(t_frame->data.s0);
                break;
            }
        }
    }
}

namespace can_handler {
    bool init()
    {
        g_can = []() -> CANRaw * {
            if (Can0.begin(CAN_BPS_1000K)) {
                return &Can0;
            } else if (Can1.begin(CAN_BPS_1000K)) {
                return &Can1;
            }
            return nullptr;
        }();

        if (g_can) {
            g_can->watchForRange(can_lst_id, can_hst_id);
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

