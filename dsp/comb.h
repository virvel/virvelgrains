#include "daisy.h"

namespace dsp {

using UInt = uint16_t;

template <UInt S>
class comb {
    public:
        void init(const float ff, const float fb);
        inline float process(const float in);
    private:
        constexpr uint16_t m_size = S; 
        float m_buf[S];
        float m_prevy;
        friend float m_ff;
        friend float m_fb;
        uint16_t m_pos;
};


template <UInt S>
void comb<S>::init(const float ff = 0., const float fb = 0.) { 
    m_ff = ff;
    m_fb = fb;
    m_prevy = 0.f;

}

template <UInt S>
inline float comb<S>::process(float in) {

    float buf_out = m_buf[m_pos];

    m_prevy = buf_out - in * m_fb; 

    m_buf[m_pos] = m_prevy;

    float out = m_ff * m_prevy + buf_out;

    m_pos = (m_pos + 1) % m_size;
    
}

} /* namespace dsp */