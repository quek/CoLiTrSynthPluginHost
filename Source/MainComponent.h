#pragma once

#include <Windows.h>
#include <JuceHeader.h>

class MainWindow;
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
		MainWindow&,
		juce::String& pluginName,
		juce::AudioPluginFormatManager& formatManager,
		juce::KnownPluginList& knownPluginList
	);
	~MainComponent() override;

	//==============================================================================
	void edit();
	void openPluginListWindow();
	void quit();
	void paint(juce::Graphics&) override;
	void resized() override;
	void getState();
	void setState();

	std::unique_ptr<juce::AudioPluginInstance> plugin;
	std::unique_ptr<EditorWindow> editorWindow;
	HANDLE hPipe;
private:
	//==============================================================================
	// Your private member variables go here...
	MainWindow& owner;
	juce::TextButton checkTheTimeButton;
	juce::Label timeLabel;

	juce::KnownPluginList& knownPluginList;
	juce::AudioPluginFormatManager& formatManager;
	juce::String pluginName_;

	void click();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
