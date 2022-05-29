#pragma once
#include <JuceHeader.h>

class MyAudioPlayHead : public juce::AudioPlayHead
{
public:
    MyAudioPlayHead();
    bool getCurrentPosition(juce::AudioPlayHead::CurrentPositionInfo& result) override;

    double bpm;
    juce::int64 timeInSamples;
};

