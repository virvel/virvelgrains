#pragma once
#include "daisy.h"
#include "daisysp.h"

namespace dsp {

    class LFSR {
        public :
            uint32_t lfsr32;
            uint32_t bit32;
        
            float getNextFloat() {
                bit32 = ((lfsr32 >> 1) ^ (lfsr32 >> 3)) & 1u;
                lfsr32 = (lfsr32 >> 1) | (bit32 << 31);
                return float(lfsr32) / (float)(std::numeric_limits<uint32_t>::max()-1.);
            }
            
            float getNext() {
                bit32 = ((lfsr32 >> 1) ^ (lfsr32 >> 3)) & 1u;
                lfsr32 = (lfsr32 >> 1) | (bit32 << 31);
                return lfsr32;
            }
    };

    class Grain
    {

    public:
        void init(float * const buffer, uint32_t size, uint32_t seed);
        float play();
        void setActive(const bool active) { m_active = active; }
        void setRate(const float rate) { m_rate = rate; }
        void setOffset(const float pos) { m_next_offset = static_cast<uint32_t>(daisysp::fmin(pos * float((m_size - m_duration)), float(m_size - m_duration))); }
        void setJitter(const float jitter) { m_jitter = jitter; }
        void setDelay(const uint32_t delayInSamples) {m_delay = delayInSamples;}
        void setDuration(const float s);
        void setSize(const uint32_t size) { m_size = size;}
        uint32_t getDuration() { return m_duration;}

    private:
        bool m_active;
        float *m_buf;
        float m_position;
        float m_prevSample;
        float m_rate;
        uint32_t m_offset;
        uint32_t m_next_offset;
        uint32_t m_duration;
        uint32_t m_size;
        float m_jitter;
        uint32_t m_delay;
        LFSR m_lfsr;
    };

    class Granulator
    {

    public:
        void init(float * const buffer, const uint32_t size);
        void setRate(const float speed);
        void setDuration(const float s);
        void setOffset(const float o);
        void setJitter(const float s);
        float play();
        void setNumSamples(const uint32_t s);
        uint32_t getNumSamples(void) { return m_numSamples; }
        LFSR lfsr;

    private:
        float *m_buffer;
        float m_spray;
        uint32_t m_numSamples;
        const uint8_t m_num_grains = 3; 
        Grain m_grains[3];
    };

}; /* namespace dsp */