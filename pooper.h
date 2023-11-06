#include "daisy.h"
#include "daisysp.h"


namespace daisysp {

    class Pooper {

        public:
            void init(float * buffer, uint32_t size);
            void process();
            void setDelayTime(float seconds);
            void setSpeed(float speed);
            void setOffset(float offset);
            const float readL();
            const float readR();
            const float readfL(float pos, uint32_t offset);
            const float readfR(float pos, uint32_t offset);
            void setNumSamples(uint32_t s) { m_numSamples = s;}
            const uint32_t getNumSamples(void) { return m_numSamples;}

        private:
            float * m_lPtr;
            float * m_rPtr;
            uint32_t m_numSamples;
            uint32_t m_length;
            float m_speed;
            float m_frac;
            float m_pos;
            uint32_t m_offset;
            float m_prevSampleL;
            float m_prevSampleR;
    };

}
