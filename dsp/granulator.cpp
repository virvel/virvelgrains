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
    
    float out = 0.f;

    if (m_position >= m_duration)
        m_active = false;

    if (m_active ) {
        uint32_t int_pos = static_cast<uint32_t>(m_position);

        float a,b,frac;

        frac = m_position - int_pos;

        auto c = float(int_pos%m_duration)/float(m_duration);
        c = sin(M_PI* c);

        a = c*m_buf[((int_pos) + m_offset) % m_size];
        b = c*m_buf[(((int_pos + 1) %  m_duration) + m_offset) % m_size];

        m_prevSample = frac * (a-m_prevSample) + b;
        out = m_prevSample;
        m_position = m_position + m_rate;
    }
    else {
        m_position = 0.f;
        m_offset = m_next_offset + m_jitter;
        m_active = true;
    }
    return out;
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

// [0,1] -> [0,size_of_buffer]
void dsp::Granulator::setDuration(const float s) {
    for (auto &g : m_grains) {
        g.setDuration(s);
    }
}

void dsp::Granulator::setRate(const float speed) {
    for(auto &g : m_grains)
        g.setRate(speed);
}

float dsp::Granulator::play() {

    auto sig = 0.f;
    for (auto &g: m_grains) {
        g.setJitter(static_cast<uint32_t>(lfsr.getNext() * m_spray));
        sig += g.play();
    }
    return sig;
}
