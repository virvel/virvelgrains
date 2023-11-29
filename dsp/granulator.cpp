#include "granulator.h"

void dsp::Grain::init(float * const  buffer, const uint32_t size) {
    m_buf = buffer;
    m_active = false; 
    m_position = 0;
    m_size = size;
    m_rate = 1.f;
    m_duration = m_size;
    m_offset = 0;
    m_next_offset = 0;
}

float dsp::Grain::play() {
    
    if (m_position >= m_duration) {
        m_active = false;
    }

    if (m_active ) {

        m_position = m_position + m_rate;

        uint32_t int_pos = static_cast<uint32_t>(m_position);
        float frac = m_position - int_pos;

        auto c = float(int_pos%m_duration)/float(m_duration);
        c = sin(M_PI* c);

        if (m_position <= 1)
            return c*m_buf[1];
        else if (m_position >= (m_size  - 2))
            return c*m_buf[m_size - 2];

        else {
            auto n = int_pos + m_offset;

            if (n > m_size - 2)
                n = m_size - 3;

            float y0 = m_buf[n-1];
            float y1 = m_buf[n];
            float y2 = m_buf[n+1];
            float y3 = m_buf[n+2];

            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c3 = 1.5f * (y1 - y2) + 0.5f * (y3 - y0);
            float c2 = y0 - y1 + c1 - c3;

            return c*(((c3 * frac + c2) * frac + c1) * frac + c0);
        }

    }
    else
    {
        m_position = 0.f;
        m_offset = m_next_offset;
        m_active = true;
        return 0.f;
    }
}

void dsp::Grain::setDuration(const float s) {
    {
        m_duration = std::max(static_cast<uint32_t>(48), static_cast<uint32_t>(s * static_cast<float>(m_size)));
    }
}

void dsp::Granulator::init(float * const buffer, const uint32_t numSamples) {
    m_numSamples = numSamples/2;
    m_buffer = buffer;

    for (auto &g : m_grains) {
        g.init(buffer, numSamples/2);
    }
}

void dsp::Granulator::setOffset(const float offset) {
    for (auto &g : m_grains) {
        g.setOffset(offset);
    }
}

void dsp::Granulator::setNumSamples(const uint32_t size) {
    m_numSamples = size;
    for(auto &g:m_grains)
        g.setSize(size);
}

void dsp::Granulator::setDuration(const float s) {
    for (auto &g : m_grains) {
        g.setDuration(s);
    }
}

void dsp::Granulator::setRate(const float speed) {
    
    m_grains[0].setRate(speed);
    m_grains[1].setRate(1.5f*speed);
    m_grains[2].setRate(2.f*speed);
    
   /*
    for(auto &g : m_grains)
        g.setRate(speed);
        */
}

void dsp::Granulator::setJitter(const float jitter) {
    for (auto &g : m_grains)
        g.setJitter(jitter);
}

float dsp::Granulator::play() {

    auto sig = 0.f;
    for (auto &g: m_grains) {
        sig += g.play();
    }
    return sig;
}
