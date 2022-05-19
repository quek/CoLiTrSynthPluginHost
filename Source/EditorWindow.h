#pragma once
#include <JuceHeader.h>

class MainComponent;

class EditorWindow : public juce::DocumentWindow
{
public:
	EditorWindow(MainComponent&, juce::String, juce::AudioProcessorEditor*);

	~EditorWindow() override;

	void closeButtonPressed() override;

private:
	MainComponent& owner;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorWindow)
};
