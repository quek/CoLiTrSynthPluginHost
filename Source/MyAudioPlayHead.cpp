#include "MyAudioPlayHead.h"
#include <cmath>

MyAudioPlayHead::MyAudioPlayHead(double sampleRate) : sampleRate_(sampleRate), bpm_(120), timeInSamples_(0)
{
}

bool MyAudioPlayHead::getCurrentPosition(juce::AudioPlayHead::CurrentPositionInfo& result)
{
	zerostruct(result);

	result.bpm = bpm_;
	result.isPlaying = isPlaying_;
	result.isRecording = false;
	result.timeSigNumerator = 4;
	result.timeSigDenominator = 4;
	result.frameRate = AudioPlayHead::fps30;
	result.timeInSamples = timeInSamples_;
	result.timeInSeconds = timeInSamples_ / sampleRate_;
	// TODO ����悭�킩��Ȃ�
	result.editOriginTime = result.timeInSeconds;

	// TODO ���ꂠ���Ă�H ����ݒ肵�Ȃ��� Piapro Studio �̍Đ����i�܂Ȃ��B
	result.ppqPosition = result.timeInSeconds * result.bpm / 60.0;
	result.ppqPositionOfLastBarStart = result.ppqPosition - std::floor(result.ppqPosition);

	result.isLooping = false;
	result.ppqLoopStart = 0;
	result.ppqLoopEnd = 0;

	return true;
}
