#include "daisy.h"
#include "daisysp.h"

namespace daisysp
{

    class Grain
    {

    public:
        Grain(){}
        ~Grain(){}
        void init(float *buffer, uint32_t size);
        const float play();
        void setActive(bool active) { m_active = active; }
        void setRate(float rate) { m_rate = rate; }
        void setPosition(float pos) { m_offset = pos; }
        void setJitter(uint32_t jitter) { m_jitter = jitter; }
        void setDuration(uint32_t s) {m_duration = s;}

    private:
        bool m_active;
        float *m_buf;
        float m_position;
        uint32_t m_offset;
        uint32_t m_duration;
        uint32_t m_size;
        uint32_t m_spread;
        uint32_t m_pan;
        uint32_t m_jitter;
        float m_rate;
    };

    class Granulator
    {

    public:
        void init(float *buffer, uint32_t size);
        void setSpeed(float speed);
        void setPosition(float position);
        void setDuration(uint32_t s);
        void setOffset(float o);
        const float play();
        void setNumSamples(uint32_t s) { m_numSamples = s; }
        const uint32_t getNumSamples(void) { return m_numSamples; }

    private:
        float *m_buffer;
        uint32_t m_numSamples;
        uint32_t m_length;
        float m_speed;
        float m_frac;
        float m_pos;
        float m_jitter;
        uint32_t m_offset;
        float m_prevSampleL;
        const uint16_t m_num_grains = 10; 
        std::default_random_engine gen;
        std::uniform_real_distribution<> dist;
        Grain m_grains[10];
    };

}
