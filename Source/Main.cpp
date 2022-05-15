/*
  ==============================================================================

	This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainWindow.h"
#include "MainComponent.h"
#include "PluginListWindow.h"

//==============================================================================
class PluginHostApplication : public juce::JUCEApplication
{
public:
	//==============================================================================
	PluginHostApplication() {}

	const juce::String getApplicationName() override { return ProjectInfo::projectName; }
	const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
	bool moreThanOneInstanceAllowed() override { return true; }

	//==============================================================================
	void initialise(const juce::String& commandLine) override
	{
		// This method is where you should put your application's initialisation code..
		juce::PropertiesFile::Options options;
		options.folderName = "CoLiTrSynth";
		options.applicationName = "Plugin Host";
		options.filenameSuffix = "settings";
		options.osxLibrarySubFolder = "Preferences";

		appProperties.reset(new juce::ApplicationProperties());
		appProperties->setStorageParameters(options);

		mainWindow.reset(new MainWindow(getApplicationName()));
	}

	void shutdown() override
	{
		// Add your application's shutdown code here..

		mainWindow = nullptr; // (deletes our window)
		appProperties = nullptr;
		juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
	}

	//==============================================================================
	void systemRequestedQuit() override
	{
		// This is called when the app is being asked to quit: you can ignore this
		// request and let the app carry on running, or call quit() to allow the app to close.
		quit();
	}

	void anotherInstanceStarted(const juce::String& commandLine) override
	{
		// When another instance of the app is launched while this one is running,
		// this method is invoked, and the commandLine parameter tells you what
		// the other instance's command-line arguments were.
	}

	//==============================================================================
	/*
		This class implements the desktop window that contains an instance of
		our MainComponent class.
	*/

	std::unique_ptr<juce::ApplicationProperties> appProperties;
private:
	std::unique_ptr<MainWindow> mainWindow;
};

static PluginHostApplication& getApp() { return *dynamic_cast<PluginHostApplication*>(juce::JUCEApplication::getInstance()); }

juce::ApplicationProperties& getAppProperties() { return *getApp().appProperties; }

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(PluginHostApplication)
