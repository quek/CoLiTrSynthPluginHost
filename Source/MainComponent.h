#pragma once

#include <JuceHeader.h>

class EditorWindow;
//==============================================================================
/*
	This component lives inside our window, and this is where you should put all
	your controls and content.
*/
class MainComponent : public juce::Component
{
public:
	//==============================================================================
	MainComponent(
		juce::String& pluginName,
		juce::AudioPluginFormatManager& formatManager,
		juce::KnownPluginList& knownPluginList
	);
	~MainComponent() override;

	//==============================================================================
	void edit();
	void paint(juce::Graphics&) override;
	void resized() override;

	std::unique_ptr<juce::AudioPluginInstance> plugin;
	std::unique_ptr<EditorWindow> editorWindow;
private:
	//==============================================================================
	// Your private member variables go here...
	juce::TextButton checkTheTimeButton;
	juce::Label timeLabel;

	juce::KnownPluginList& knownPluginList;
	juce::AudioPluginFormatManager& formatManager;

	void click();
	void play();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
