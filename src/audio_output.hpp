#pragma once
#include <cstddef>
#include <cstdint>
namespace arcade {
class AudioOutput {
public:
 virtual ~AudioOutput()=default;
 virtual bool configure(unsigned sample_rate)=0;
 virtual void enqueue(const std::int16_t* stereo,std::size_t frames)=0;
 virtual void pause(bool)=0;
 virtual void stop()=0;
};
}
