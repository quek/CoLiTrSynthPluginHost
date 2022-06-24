#pragma once
#include <JuceHeader.h>

class MyAudioPlayHead : public juce::AudioPlayHead
{
public:
    MyAudioPlayHead(double sampleRate);
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override;

    double sampleRate_;
    bool isPlaying_;
    double bpm_;
    juce::int64 timeInSamples_;
};

