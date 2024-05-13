#pragma once
//==============================================================================
#include "dsp/data/AudioRingBuffer.h"
#include "gui/widget/Graph.h"
#include <JuceHeader.h>
//==============================================================================
namespace dmt {
namespace gui {
namespace component {
//==============================================================================
template<typename SampleType>
class OscilloscopeComponent : public juce::Component
{
  using Graph = dmt::gui::widget::Graph<SampleType>;

public:
  //==============================================================================
  OscilloscopeComponent()
    : graph([this](int numDataPoints) {
      return this->getLeftChannelAmplitudes(numDataPoints);
    })
  {
  }

  //==============================================================================
  void paint(juce::Graphics&) override {}
  void resized() override {}
  //==============================================================================
  const SampleType* getLeftChannelAmplitudes(int /*numDataPoints*/)
  {
    return {};
  }
  //==============================================================================
private:
  Graph graph;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeComponent)
};
} // namespace component
} // namespace gui
} // namespace dmt