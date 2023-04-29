#include "daisy.h"
#include "daisysp.h"


namespace daisysp {

    class Pooper {

        public:
            void init(float * buffer, int32_t size);
            void process();
            void setDelayTime(float seconds);
            void setSpeed(float speed);
            void setOffset(float offset);
            const float readf(float pos);
            const float read();

        private:
            float * m_buffer;
            float * m_rPtr;
            int32_t m_numSamples;
            uint32_t m_length;
            float m_delayTime;
            float m_speed;
            float m_frac;
            float m_pos;
            int32_t m_offset;
            float prevSample;
            float coeffs[4];
    };

}
