//==============================================================================
#pragma once
//==============================================================================
#include "../data/RingBuffer.h"
#include <JuceHeader.h>
//==============================================================================
namespace dmt {
namespace dsp {
namespace processor {
//==============================================================================
template<typename SampleType>
class OscilloscopeProcessor
{
  using RingBuffer = dmt::dsp::data::RingBuffer<SampleType>;
  using BufferData = std::vector<std::vector<SampleType>>;
  using ChannelData = std::vector<SampleType>;

public:
  //============================================================================
  ~OscilloscopeProcessor() noexcept
  {
    kill = true;

    std::unique_lock<std::mutex> lock(closeMutex);
    closeConditionVariable.wait(lock,
                                [this] { return this->closed.get() == true; });
    lock.unlock();
  }
  //============================================================================
  void prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/) noexcept
  {
    ringBuffer =
      std::make_unique<RingBuffer>(ringBufferNumChannels, ringBufferNumSamples);
    transferThread =
      std::thread(&OscilloscopeProcessor::transferThreadCallback, this);
    transferThread.detach();
  }
  //============================================================================
  void processBlock(juce::AudioBuffer<SampleType>& buffer) noexcept
  {
    juce::AudioBuffer<SampleType> bufferCopy(buffer);
    bufferQueue.push(bufferCopy);
    queueConditionVariable.notify_one();
  }
  //============================================================================
  void transferThreadCallback() noexcept
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    while (!kill.get()) {
      queueConditionVariable.wait(lock, [this] {
        return !this->bufferQueue.empty() || this->kill.get();
      });
      while (!bufferQueue.empty()) {
        // Copy the front
        const auto& buffer = bufferQueue.front();
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        // Allocate a vector of vectors to store the audio data
        std::vector<std::vector<SampleType>> data(
          numChannels, std::vector<SampleType>(numSamples));

        // Copy the audio data from the buffer into the vector of vectors
        for (int channel = 0; channel < numChannels; ++channel) {
          auto* channelData = buffer.getReadPointer(channel);
          for (int sample = 0; sample < numSamples; ++sample) {
            data[channel][sample] = channelData[sample];
          }
        }

        // Write the audio data to the ring buffer
        ringBuffer->write(data);

        // Pop the front of the queue
        bufferQueue.pop();
      }
    }
    lock.unlock();
    closed = true;
    closeConditionVariable.notify_one();
  }
  //============================================================================
  const BufferData getAmplitudes(const int numDataPoints) noexcept
  {
    // Buffer with the original audio data
    auto buffer = ringBuffer->read();

    // Return the buffer with the downsampled audio data
    BufferData data(ringBufferNumChannels, ChannelData(numDataPoints));

    // Downsample the audio data using a lookup table
    for (int channel = 0; channel < ringBufferNumChannels; ++channel) {
      // Create a lookup table for the audio data
      juce::dsp::LookupTable<SampleType> lookupTable(
        [buffer, channel](int x) { return buffer[channel][x]; },
        ringBufferNumSamples);
      // Downsample the audio data
      for (int dataPoint = 0; dataPoint < numDataPoints; ++dataPoint) {
        const auto normalizedIndex = (float)dataPoint / (float)numDataPoints;
        const auto index = normalizedIndex * (float)ringBufferNumSamples;
        const auto value = lookupTable.get(index);
        data[channel][dataPoint] = value;
      }
    }

    return data;
  }
  //============================================================================

  const int ringBufferNumChannels = 2;
  const int ringBufferNumSamples = 4096;
  std::shared_ptr<RingBuffer> ringBuffer;
  std::queue<juce::AudioBuffer<SampleType>> bufferQueue;
  std::condition_variable queueConditionVariable;
  std::condition_variable closeConditionVariable;
  std::mutex queueMutex;
  std::mutex closeMutex;
  std::thread transferThread;
  juce::Atomic<bool> kill{ false };
  juce::Atomic<bool> closed{ true };
};
} // namespace processor
} // namespace dsp
} // namespace dmt