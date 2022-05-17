/*
  ==============================================================================

	This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainWindow.h"
#include "MainComponent.h"
#include "PluginListWindow.h"


class PluginScannerSubprocess : private juce::ChildProcessWorker,
	private juce::AsyncUpdater
{
public:
	using ChildProcessWorker::initialiseFromCommandLine;

private:
	void handleMessageFromCoordinator(const juce::MemoryBlock& mb) override
	{
		if (mb.isEmpty())
			return;

		if (!doScan(mb))
		{
			{
				const std::lock_guard<std::mutex> lock(mutex);
				pendingBlocks.emplace(mb);
			}

			triggerAsyncUpdate();
		}
	}

	void handleConnectionLost() override
	{
		juce::JUCEApplicationBase::quit();
	}

	void handleAsyncUpdate() override
	{
		for (;;)
		{
			const auto block = [&]() -> juce::MemoryBlock
			{
				const std::lock_guard<std::mutex> lock(mutex);

				if (pendingBlocks.empty())
					return {};

				auto out = std::move(pendingBlocks.front());
				pendingBlocks.pop();
				return out;
			}();

			if (block.isEmpty())
				return;

			doScan(block);
		}
	}

	bool doScan(const juce::MemoryBlock& block)
	{
		juce::AudioPluginFormatManager formatManager;
		formatManager.addDefaultFormats();

		juce::MemoryInputStream stream{ block, false };
		const auto formatName = stream.readString();
		const auto identifier = stream.readString();

		juce::PluginDescription pd;
		pd.fileOrIdentifier = identifier;
		pd.uniqueId = pd.deprecatedUid = 0;

		const auto matchingFormat = [&]() -> juce::AudioPluginFormat*
		{
			for (auto* format : formatManager.getFormats())
				if (format->getName() == formatName)
					return format;

			return nullptr;
		}();

		if (matchingFormat == nullptr
			|| (!juce::MessageManager::getInstance()->isThisTheMessageThread()
				&& !matchingFormat->requiresUnblockedMessageThreadDuringCreation(pd)))
		{
			return false;
		}

		juce::OwnedArray<juce::PluginDescription> results;
		matchingFormat->findAllTypesForFile(results, identifier);
		sendPluginDescriptions(results);
		return true;
	}

	void sendPluginDescriptions(const juce::OwnedArray<juce::PluginDescription>& results)
	{
		juce::XmlElement xml("LIST");

		for (const auto& desc : results)
			xml.addChildElement(desc->createXml().release());

		const auto str = xml.toString();
		sendMessageToCoordinator({ str.toRawUTF8(), str.getNumBytesAsUTF8() });
	}

	std::mutex mutex;
	std::queue<juce::MemoryBlock> pendingBlocks;
};

//==============================================================================
class PluginHostApplication : public juce::JUCEApplication, private juce::AsyncUpdater
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
		auto scannerSubprocess = std::make_unique<PluginScannerSubprocess>();

		if (scannerSubprocess->initialiseFromCommandLine(commandLine, processUID))
		{
			storedScannerSubprocess = std::move(scannerSubprocess);
			return;
		}

		// This method is where you should put your application's initialisation code..
		juce::PropertiesFile::Options options;
		options.folderName = "CoLiTrSynth";
		options.applicationName = "Plugin Host";
		options.filenameSuffix = "settings";
		options.osxLibrarySubFolder = "Preferences";

		appProperties.reset(new juce::ApplicationProperties());
		appProperties->setStorageParameters(options);

		mainWindow.reset(new MainWindow());
	}

	void handleAsyncUpdate() override
	{
        juce::File fileToOpen;

       #if JUCE_ANDROID || JUCE_IOS
        fileToOpen = PluginGraph::getDefaultGraphDocumentOnMobile();
       #else
        for (int i = 0; i < getCommandLineParameterArray().size(); ++i)
        {
            fileToOpen = juce::File::getCurrentWorkingDirectory().getChildFile (getCommandLineParameterArray()[i]);

            if (fileToOpen.existsAsFile())
                break;
        }
       #endif

        if (! fileToOpen.existsAsFile())
        {
            juce::RecentlyOpenedFilesList recentFiles;
            recentFiles.restoreFromString (getAppProperties().getUserSettings()->getValue ("recentFilterGraphFiles"));

            if (recentFiles.getNumFiles() > 0)
                fileToOpen = recentFiles.getFile (0);
        }

        //if (fileToOpen.existsAsFile())
        //    if (auto* graph = mainWindow->graphHolder.get())
        //        if (auto* ioGraph = graph->graph.get())
        //            ioGraph->loadFrom (fileToOpen, true);
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

	void anotherInstanceStarted(const juce::String& ) override
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
	std::unique_ptr<PluginScannerSubprocess> storedScannerSubprocess;
};

static PluginHostApplication& getApp() { return *dynamic_cast<PluginHostApplication*>(juce::JUCEApplication::getInstance()); }

juce::ApplicationProperties& getAppProperties() { return *getApp().appProperties; }

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(PluginHostApplication)
