#pragma once
#include <JuceHeader.h>
#include "MainComponent.h"

class PluginListWindow;

class MainWindow : public juce::DocumentWindow
{
public:
	MainWindow(juce::String name)
		: DocumentWindow(name,
			juce::Desktop::getInstance().getDefaultLookAndFeel()
			.findColour(juce::ResizableWindow::backgroundColourId),
			DocumentWindow::allButtons)
	{
		setUsingNativeTitleBar(true);
		setContentOwned(new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
		setFullScreen(true);
#else
		setResizable(true, true);
		centreWithSize(getWidth(), getHeight());
#endif

		setTopLeftPosition(60, 60);
		setVisible(true);
	}

	void closeButtonPressed() override
	{
		// This is called when the user tries to close this window. Here, we'll just
		// ask the app to quit when this happens, but you can change this to do
		// whatever you need.
		juce::JUCEApplication::getInstance()->systemRequestedQuit();
	}

	/* Note: Be careful if you override any DocumentWindow methods - the base
	   class uses a lot of them, so by overriding you might break its functionality.
	   It's best to do all your work in your content component instead, but if
	   you really have to override any DocumentWindow methods, make sure your
	   subclass also calls the superclass's method.
	*/
	juce::KnownPluginList knownPluginList;
	std::unique_ptr<PluginListWindow> pluginListWindow;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

