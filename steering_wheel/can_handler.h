#ifndef can_handler_h
#define can_handler_h

#include <due_can.h>
#include <RingBuf.h>

namespace can_handler {
    RingBuf<CAN_FRAME, 64> m_ring;
}

// hidden
namespace {
    CANRaw *g_can{nullptr};
    constexpr uint32_t can_lst_id{0x600};
    constexpr uint32_t can_hst_id{0x625};

    // Here we receive frames, we filter fast and add
    // it to the ring
    void routine(CAN_FRAME *t_frame)
    {
        // TODO: This is an example to remember
        //       the logic.
        switch(t_frame->id) {
            case 0x600:
            case 0x601: {
                can_handler::m_ring.push(*t_frame);
            }
        }
    }
}

namespace can_handler {
    bool init()
    {
        g_can = []() -> CANRaw * {
            if (Can0.begin(CAN_BPS_1000K, 62)) {
                return &Can0;
            } else if (Can1.begin(CAN_BPS_1000K, 65)) {
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

