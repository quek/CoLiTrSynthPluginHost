#include "MyAudioPlayHead.h"
#include <cmath>

MyAudioPlayHead::MyAudioPlayHead(double sampleRate) : sampleRate_(sampleRate), bpm_(120), timeInSamples_(0)
{
}

juce::Optional<juce::AudioPlayHead::PositionInfo> MyAudioPlayHead::getPosition() const
{
	auto position = juce::AudioPlayHead::PositionInfo();

	position.setBpm(bpm_);
	position.setIsPlaying(isPlaying_);
	position.setIsRecording(false);

	auto ts = juce::AudioPlayHead::TimeSignature();
	ts.numerator = 4;
	ts.denominator = 4;
	position.setTimeSignature(ts);
	position.setFrameRate(juce::AudioPlayHead::fps30);
	position.setTimeInSamples(timeInSamples_);
	auto timeInSeconds = timeInSamples_ / sampleRate_;
	position.setTimeInSeconds(timeInSeconds);
	position.setEditOriginTime(timeInSeconds);

	auto  ppqPosition = timeInSeconds * bpm_ / 60.0;
	position.setPpqPosition(ppqPosition);
	position.setPpqPositionOfLastBarStart(ppqPosition - std::floor(ppqPosition));

	position.setIsLooping(false);
	auto loopPoints = juce::AudioPlayHead::LoopPoints();
	loopPoints.ppqStart = 0.0;
	loopPoints.ppqEnd = 0.0;
	position.setLoopPoints(loopPoints);

	return position;
}
