#include "MyAudioPlayHead.h"
#include <cmath>

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
	// TODO Ç±ÇÍÇÊÇ≠ÇÌÇ©ÇÒÇ»Ç¢
	result.editOriginTime = result.timeInSeconds;

	// TODO Ç±ÇÍÇ†Ç¡ÇƒÇÈÅH Ç±ÇÍê›íËÇµÇ»Ç¢Ç∆ Piapro Studio ÇÃçƒê∂Ç™êiÇ‹Ç»Ç¢ÅB
	result.ppqPosition = result.timeInSeconds * result.bpm / 60.0;
	result.ppqPositionOfLastBarStart = result.ppqPosition - std::floor(result.ppqPosition);
	result.ppqLoopStart = 0;
	result.ppqLoopEnd = 0;

	return true;
}
