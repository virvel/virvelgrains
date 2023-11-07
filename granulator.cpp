#include <random>
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

    /*if (int_pos >= m_duration)
    {
        m_active = false;
        out = 0.f;
    }

    if (m_active) 
    {
        */
        float a,b,frac;

        frac = m_position - int_pos;

        a = m_buf[((int_pos % m_duration) + (m_offset + m_jitter)) % m_size];
        b = m_buf[(((int_pos + 1) %  m_duration) + (m_offset + m_jitter)) % m_size]; 

        auto c = float(int_pos%m_duration)/float(m_duration);
        if (c > 0.5)
            c = 1. -c;
        c = sin(sin(M_PI_2 * c));

        out= c*a+(b-a)*frac;
   // }
    m_position = fmod(m_position + m_rate, m_duration);
    return out;
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

    for (int i = 0; i < m_num_grains; ++i) {
        m_grains[i].setRate(float(i)/8.f);
    }
}

// [0,1] -> [0,numSamples-length]
void Granulator::setOffset(float offset) {
    m_offset = fmin(offset * float( m_numSamples - m_length), float( m_numSamples - m_length));
}

void Granulator::setPosition(float position) {
    m_pos = position;
    float jitter; 
    for (auto &g : m_grains) {
        jitter = static_cast<uint32_t>(1000.f*dist(gen));
        g.setJitter(jitter);
        g.setPosition(position);
    }
}

// Set duration in samples for each individual grain
void Granulator::setDuration(uint32_t s) {
    for (auto &g : m_grains) {
        g.setDuration(s);
    }
}

void Granulator::setSpeed(float speed) {
    for (int i = 0; i < m_num_grains; ++i) {
        m_grains[i].setRate(speed * float(i) / m_num_grains);
    }
//    for (auto &g : m_grains)
//        g.setRate(speed);
}

const float Granulator::play() {

    auto sig = 0.f;
    for (auto &g: m_grains) {
        sig += g.play();
    }
    return sig;
}