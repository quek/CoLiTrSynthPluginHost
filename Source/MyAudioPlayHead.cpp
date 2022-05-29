#include "MyAudioPlayHead.h"

MyAudioPlayHead::MyAudioPlayHead() : bpm(120), timeInSamples(0)
{
}

bool MyAudioPlayHead::getCurrentPosition(juce::AudioPlayHead::CurrentPositionInfo& result)
{
	zerostruct(result);

	result.bpm = bpm;
	result.isPlaying = isPlaying;
	result.isRecording = false;
	result.isLooping = false;
	result.timeSigNumerator = 4;
	result.timeSigDenominator = 4;
	result.frameRate = AudioPlayHead::fps30;
	result.timeInSamples = timeInSamples;
	result.timeInSeconds = timeInSamples / 48000.0;
	// TODO これよくわかんない
	result.editOriginTime = result.timeInSeconds;

	// TODO これあってる？ これ設定しないと Piapro Studio の再生が進まない。
	result.ppqPosition = result.timeInSeconds * result.bpm / 60.0;
	result.ppqPositionOfLastBarStart = 0;
	result.ppqLoopStart = 0;
	result.ppqLoopEnd = 0;

	return true;
}
