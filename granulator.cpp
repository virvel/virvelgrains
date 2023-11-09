#include "granulator.h"

using namespace daisysp;

void Grain::init(float * buffer, uint32_t size) {
    m_buf = buffer;
    m_active = false; 
    m_position = 0;
    m_size = size;
    m_rate = 1.f;
    m_duration = m_size;
    m_offset = 0;
}

const float Grain::play() {
    float out;
    uint32_t int_pos = static_cast<uint32_t>(m_position);

    float a,b,frac;

    frac = m_position - int_pos;

    auto c = float(int_pos%m_duration)/float(m_duration);
    c = sin(M_PI* c);

    a = c*m_buf[((int_pos) + m_offset) % m_size];
    b = c*m_buf[(((int_pos + 1) %  m_duration) + m_offset) % m_size];


    out= a+(b-a)*frac;
    m_position = fmod(m_position + m_rate, float(m_duration));
    return out;
}

void Grain::setDuration(float s) {
    {
        m_duration = std::max(static_cast<uint32_t>(48), static_cast<uint32_t>(s * static_cast<float>(m_size)));
        //m_duration = std::min(m_duration, m_size-m_offset);
    }
}



void Granulator::init(float * buffer, uint32_t numSamples) {
    m_numSamples = numSamples/2;
    m_buffer = buffer;
    m_length = numSamples/2;
    m_speed = 1.0f;
    m_prevSampleL = 0.f;
    dist = std::uniform_real_distribution<>(0.f, 1.f);

    for (auto &g : m_grains) {
        g.init(buffer, numSamples/2);
    }
    m_grains[0].setRate(1.f);
    m_grains[1].setRate(1.5f);
    m_grains[2].setRate(2.f);
}


void Granulator::setOffset(float offset) {
    float jitter;
    for (auto &g : m_grains) {
        jitter = static_cast<uint32_t>(1000.f*dist(gen));
        g.setJitter(jitter);
        g.setOffset(offset);
    }
}

void Granulator::setNumSamples(uint32_t size) {
    m_numSamples = size;
    for(auto &g:m_grains)
        g.setSize(size);
}

// Set duration in samples for each individual grain
void Granulator::setDuration(float s) {
    for (auto &g : m_grains) {
        g.setDuration(s);
    }
}

void Granulator::setSpeed(float speed) {
    m_grains[0].setRate(speed);
    m_grains[1].setRate(speed*1.5);
    m_grains[2].setRate(speed*2.f);
}

const float Granulator::play() {

    auto sig = 0.f;
    for (auto &g: m_grains) {
        sig += g.play();
    }
    return sig;
}