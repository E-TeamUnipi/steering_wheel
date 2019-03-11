#ifndef display_h
#define display_h

// hidden
namespace {

    // send to uart serial port data in this format:
    // 
    // |        delimiter       |   id   |     data       |   crc == id    |
    // |         3 bytes        | 1 byte |    2 bytes     |    2 bytes     |
    //
    // crc for now not used, we need it fastest as possible,
    // if there are errors it isn't a problem because error converge
    // fastly to the right value. 
    struct __attribute__((packed)) display_msg {
        uint8_t m_delimiter[3];
        uint8_t m_id;
        uint16_t m_msg;
        uint16_t crc;

        display_msg(uint8_t t_id, uint16_t t_msg) :
            m_delimiter{0xff, 0xff, 0xff},
            m_id(t_id), m_msg(t_msg), crc{t_id} {}
    };
}

namespace display {
    inline
    void send(uint8_t msg_id, uint16_t msg)
    {
        display_msg buf(msg_id, msg);
        Serial.write(reinterpret_cast<uint8_t *>(&buf), 6);
    }
}

#endif

