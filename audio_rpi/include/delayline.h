/*
Copyright (c) 2020 Electrosmith, Corp
MIT License — https://opensource.org/licenses/MIT
Extracted from DaisySP for standalone use on Linux/RPi.
*/
#pragma once
#ifndef DSY_DELAYLINE_H
#define DSY_DELAYLINE_H

#include <stdint.h>
#include <cstring>

namespace daisysp
{
template <typename T, size_t max_size>
class DelayLine
{
  public:
    DelayLine() {}
    ~DelayLine() {}

    void Init()
    {
        std::memset(line_, 0, sizeof(line_));
        write_ptr_ = 0;
        delay_     = 1;
    }

    void SetDelay(size_t delay)
    {
        if(delay >= max_size) delay = max_size - 1;
        delay_ = delay;
    }

    void SetDelay(float delay)
    {
        frac_  = delay - static_cast<size_t>(delay);
        size_t int_delay = static_cast<size_t>(delay);
        if(int_delay >= max_size) int_delay = max_size - 1;
        delay_ = int_delay;
    }

    void Write(const T sample)
    {
        line_[write_ptr_] = sample;
        write_ptr_ = (write_ptr_ - 1 + max_size) % max_size;
    }

    const T Read() const
    {
        T a = line_[(write_ptr_ + delay_) % max_size];
        T b = line_[(write_ptr_ + delay_ + 1) % max_size];
        return a + (b - a) * frac_;
    }

    const T Read(float delay) const
    {
        size_t int_d = static_cast<size_t>(delay);
        float  frac  = delay - int_d;
        T a = line_[(write_ptr_ + int_d) % max_size];
        T b = line_[(write_ptr_ + int_d + 1) % max_size];
        return a + (b - a) * frac;
    }

  private:
    T      line_[max_size];
    float  frac_      = 0.f;
    size_t write_ptr_ = 0;
    size_t delay_     = 1;
};
} // namespace daisysp
#endif
