#pragma once
#include <JuceHeader.h>
#include "Main.h"
#include "MainWindow.h"
#include "CustomPluginListComponent.h"

class PluginListWindow : public juce::DocumentWindow
{
public:
	PluginListWindow(MainWindow& mw, juce::AudioPluginFormatManager& pluginFormatManager)
		: DocumentWindow("Available Plugins",
			juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
			juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton),
		owner(mw)
	{
		auto deadMansPedalFile = getAppProperties().getUserSettings()
			->getFile().getSiblingFile("RecentlyCrashedPluginsList");

		setContentOwned(new CustomPluginListComponent(pluginFormatManager,
			owner.knownPluginList,
			deadMansPedalFile,
			getAppProperties().getUserSettings(),
			true), true);

		setResizable(true, false);
		setResizeLimits(300, 400, 800, 1500);
		setTopLeftPosition(60, 60);

		restoreWindowStateFromString(getAppProperties().getUserSettings()->getValue("listWindowPos"));
		setVisible(true);
	}

	~PluginListWindow() override
	{
		getAppProperties().getUserSettings()->setValue("listWindowPos", getWindowStateAsString());
		clearContentComponent();
	}

	void closeButtonPressed() override
	{
		owner.pluginListWindow = nullptr;
	}

private:
	MainWindow& owner;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginListWindow)
};
