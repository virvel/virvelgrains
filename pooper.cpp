#include "pooper.h"


using namespace daisysp;

void Pooper::init(float * buffer, uint32_t numSamples) {
    m_numSamples = numSamples/2;
    m_rPtr = buffer;
    m_lPtr = &buffer[numSamples/2];
    m_length = numSamples/2;
    m_speed = 1.0f;
    m_prevSampleL = 0.f;
    m_prevSampleR = 0.f;
}

void Pooper::process() {
    m_pos = fmod(m_pos + m_speed, m_length);
}

void Pooper::setDelayTime(float t) {
    float floatSamples = static_cast<float>(m_numSamples);
    m_length = std::max(static_cast<uint32_t>(48),static_cast<uint32_t>(t* floatSamples));
}


void Pooper::setSpeed(float speed) {
    m_speed = speed;
}

void Pooper::setOffset(float offset) {
    #if 0
    m_offset = fmin(offset * float( m_numSamples - m_length), float( m_numSamples - m_length));
    #else
    float offs = fmin(offset * float( m_numSamples - m_length), float( m_numSamples - m_length));
    auto int_offs = static_cast<uint32_t>(offs);
    auto frac = offs - static_cast<float>(int_offs);
    m_offset = m_offset + static_cast<uint32_t>(frac*static_cast<float>(int_offs - m_offset));
    #endif
}

const float Pooper::readL() {
    float pos = m_pos;
    int32_t offset = m_offset;
    return readfL(pos, offset);
}

inline const float Pooper::readfL(float pos, uint32_t offset) {
    float a,b,frac;

    uint32_t int_pos = static_cast<uint32_t>(pos);
    frac = pos - int_pos;

    auto c = float(int_pos%m_length)/float(m_length);
    if (c > 0.5)
        c = 1. -c;
    c = sin(sin(M_PI * c));

    a = c*m_lPtr[(int_pos + offset) % m_length];
    b = c*m_lPtr[(int_pos + 1 + offset) % m_length];
    m_prevSampleL = frac * (a-m_prevSampleL) + b;

    return m_prevSampleL;
}

const float Pooper::readR() {
    float pos = m_pos;
    uint32_t offset = m_offset;
    return readfR(pos, offset);
}

inline const float Pooper::readfR(float pos, uint32_t offset) {
    uint32_t int_pos = static_cast<uint32_t>(pos);
    auto c = float(int_pos%m_length)/float(m_length);
    if (c > 0.5)
        c = 1. -c;
    c = sin(sin(M_PI * c));

    float frac = pos- int_pos;
    float a = c*m_rPtr[(int_pos + offset) % m_length];
    float b = c*m_rPtr[(int_pos + 1 + offset) % m_length];
    m_prevSampleR = frac * (a-m_prevSampleR) + b;


    return c*m_prevSampleR;
}

