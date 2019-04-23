#ifndef click_handler_h
#define click_handler_h

#include <Arduino.h>
#include <RingBuf.h>

#ifndef SOFTWARE_DEBOUNCING
#define SOFTWARE_DEBOUNCING 1
#endif

namespace click_handler
{
    static constexpr struct {
        const uint8_t upshift{22};
        const uint8_t downshift{23};
    } pins;

    // I suppose and hope that the driver is not crazy, so a ring
    // of size 32 is big enough: to overflow the driver need to
    // click buttons really fast, beyond human capabilities :/
    RingBuf<uint8_t, 32> m_ring;
}

// hidden
namespace {
    // Every button have the same logic, so I use a template
    // to have different handler for different pins.
    // This function implement also a fast debounce algorithm that
    // to remove spurious interrupts: to work properly is needed
    // that interrupt mode is setted to CHANGE.
    // P.S. Software debouncing can be disabled at compile time 
    template <uint8_t pin> void handler()
    {
        #if SOFTWARE_DEBOUNCING == 1
        volatile static bool clicked = false;
        static uint64_t last_time = 0;
        uint64_t curr_time = millis();

        if (!clicked && curr_time - last_time > 40) {
            // the handler works with interrupt disabled, so
            // is safe to not lock the push
            click_handler::m_ring.push(pin);
        }
        clicked = digitalRead(pin) == LOW;
    
        last_time = curr_time;
        #else
        click_handler::m_ring.push(pin);
        #endif
    }

    // Magic, compile time efficient init.
    template<size_t N> void init_interrupts()
    {
        constexpr uint8_t *p{(uint8_t *)&click_handler::pins};
        pinMode(p[N-1], INPUT_PULLUP);
        #if SOFTWARE_DEBOUNCING == 1
        attachInterrupt(digitalPinToInterrupt(p[N-1]), handler<p[N-1]>, CHANGE);
        #else
        attachInterrupt(digitalPinToInterrupt(p[N-1]), handler<p[N-1]>, FALLING);
        #endif

        init_interrupts<N-1>();
    }
    template<> void init_interrupts<0>(){}
}

namespace click_handler
{
    // TODO: find a better way to call init_interrupts.
    void init()
    {
        init_interrupts<sizeof(pins)>();
    }
    
}

#endif

