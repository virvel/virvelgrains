#include "comb.h"

namespace dsp {

template <UInt S>
class allpass : comb<S> {

    public: 
        void init(float g);
    
};

template <UInt S>
void allpass<S>::init(float g) {
    m_ff = g;
    m_fb = g;
}

} /* namespace dsp */