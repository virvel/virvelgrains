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
        void setOffset(float pos) { m_offset = static_cast<uint32_t>(fmin(pos * float((m_size - m_offset)), float(m_size - m_offset))); }
        void setJitter(uint32_t jitter) { m_jitter = jitter; }
        void setDuration(float s);
        void setSize(uint32_t size) { m_size = size;}

    private:
        bool m_active;
        float *m_buf;
        float m_position;
        float m_prevSample;
        uint32_t m_offset;
        uint32_t m_duration;
        uint32_t m_size;
        uint32_t m_jitter;
        float m_rate;
    };

    class Granulator
    {

    public:
        void init(float *buffer, uint32_t size);
        void setSpeed(float speed);
        void setDuration(float s);
        void setOffset(float o);
        const float play();
        void setNumSamples(uint32_t s);
        const uint32_t getNumSamples(void) { return m_numSamples; }

    private:
        float *m_buffer;
        uint32_t m_numSamples;
        const uint16_t m_num_grains = 3; 
        std::default_random_engine gen;
        std::uniform_real_distribution<> dist;
        Grain m_grains[3];
    };

}
