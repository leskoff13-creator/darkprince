#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Простой питч-шифтер на двух плавающих линиях задержки (кроссфейд окном Ханна).
// ratio < 1 -> ниже, ratio > 1 -> выше. Заодно "утяжеляет" форманты, что нам и нужно.
class PitchShifter
{
public:
    void prepare (double sampleRate, float windowMs)
    {
        windowSamples = (float) (sampleRate * windowMs * 0.001);
        int size = 1;
        while (size < (int) windowSamples * 2 + 8) size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        reset();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        phase = 0.0f;
    }

    float getLatencySamples() const { return windowSamples * 0.5f + 1.0f; }

    float process (float in, float ratio) noexcept
    {
        buffer[(size_t) writePos] = in;
        float out = 0.0f;

        for (int k = 0; k < 2; ++k)
        {
            float p = phase + 0.5f * (float) k;
            if (p >= 1.0f) p -= 1.0f;

            const float delay = p * windowSamples + 1.0f;
            const float w = 0.5f - 0.5f * std::cos (6.2831853f * p);
            out += w * readInterp (delay);
        }

        phase += (1.0f - ratio) / windowSamples;
        phase -= std::floor (phase);
        writePos = (writePos + 1) & mask;
        return out;
    }

private:
    float readInterp (float delay) const noexcept
    {
        const float pos = (float) writePos - delay;
        const int i0 = (int) std::floor (pos);
        const float frac = pos - (float) i0;
        const float a = buffer[(size_t) (i0 & mask)];
        const float b = buffer[(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * frac;
    }

    std::vector<float> buffer;
    int mask = 0, writePos = 0;
    float windowSamples = 1.0f, phase = 0.0f;
};

// Задержка для "сухого" сигнала, чтобы он совпадал по фазе с обработанным.
class DryDelay
{
public:
    void prepare (int delaySamples)
    {
        d = std::max (1, delaySamples);
        buf.assign ((size_t) d + 1, 0.0f);
        pos = 0;
    }
    float process (float in) noexcept
    {
        buf[(size_t) pos] = in;
        pos = (pos + 1) % (int) buf.size();
        return buf[(size_t) pos];
    }
private:
    std::vector<float> buf;
    int d = 1, pos = 0;
};
