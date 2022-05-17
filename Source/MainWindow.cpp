#include "MainWindow.h"
#include "PluginListWindow.h"

class CustomPluginScanner : public juce::KnownPluginList::CustomScanner,
	private juce::ChangeListener
{
public:
	CustomPluginScanner()
	{
		if (auto* file = getAppProperties().getUserSettings())
			file->addChangeListener(this);

		changeListenerCallback(nullptr);
	}

	~CustomPluginScanner() override
	{
		if (auto* file = getAppProperties().getUserSettings())
			file->removeChangeListener(this);
	}

	bool findPluginTypesFor(juce::AudioPluginFormat& format,
		juce::OwnedArray<juce::PluginDescription>& result,
		const juce::String& fileOrIdentifier) override
	{
		if (scanInProcess)
		{
			superprocess = nullptr;
			format.findAllTypesForFile(result, fileOrIdentifier);
			return true;
		}

		if (superprocess == nullptr)
		{
			superprocess = std::make_unique<Superprocess>(*this);

			std::unique_lock<std::mutex> lock(mutex);
			connectionLost = false;
		}

		juce::MemoryBlock block;
		juce::MemoryOutputStream stream{ block, true };
		stream.writeString(format.getName());
		stream.writeString(fileOrIdentifier);

		if (superprocess->sendMessageToWorker(block))
		{
			std::unique_lock<std::mutex> lock(mutex);
			gotResponse = false;
			pluginDescription = nullptr;

			for (;;)
			{
				if (condvar.wait_for(lock,
					std::chrono::milliseconds(50),
					[this] { return gotResponse || shouldExit(); }))
				{
					break;
				}
			}

			if (shouldExit())
			{
				superprocess = nullptr;
				return true;
			}

			if (connectionLost)
			{
				superprocess = nullptr;
				return false;
			}

			if (pluginDescription != nullptr)
			{
				for (const auto* item : pluginDescription->getChildIterator())
				{
					auto desc = std::make_unique<juce::PluginDescription>();

					if (desc->loadFromXml(*item))
						result.add(std::move(desc));
				}
			}

			return true;
		}

		superprocess = nullptr;
		return false;
	}

	void scanFinished() override
	{
		superprocess = nullptr;
	}

private:
	class Superprocess : private juce::ChildProcessCoordinator
	{
	public:
		explicit Superprocess(CustomPluginScanner& o)
			: owner(o)
		{
			launchWorkerProcess(juce::File::getSpecialLocation(juce::File::currentExecutableFile), processUID, 0, 0);
		}

		using juce::ChildProcessCoordinator::sendMessageToWorker;

	private:
		void handleMessageFromWorker(const juce::MemoryBlock& mb) override
		{
			auto xml = parseXML(mb.toString());

			const std::lock_guard<std::mutex> lock(owner.mutex);
			owner.pluginDescription = std::move(xml);
			owner.gotResponse = true;
			owner.condvar.notify_one();
		}

		void handleConnectionLost() override
		{
			const std::lock_guard<std::mutex> lock(owner.mutex);
			owner.pluginDescription = nullptr;
			owner.gotResponse = true;
			owner.connectionLost = true;
			owner.condvar.notify_one();
		}

		CustomPluginScanner& owner;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Superprocess)
	};

	void changeListenerCallback(juce::ChangeBroadcaster*) override
	{
		if (auto* file = getAppProperties().getUserSettings())
			scanInProcess = (file->getIntValue(scanModeKey) == 0);
	}

	std::unique_ptr<Superprocess> superprocess;
	std::mutex mutex;
	std::condition_variable condvar;
	std::unique_ptr<juce::XmlElement> pluginDescription;
	bool gotResponse = false;
	bool connectionLost = false;

	std::atomic<bool> scanInProcess{ true };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomPluginScanner)
};

MainWindow::MainWindow()
	: juce::DocumentWindow(juce::JUCEApplication::getInstance()->getApplicationName(),
		juce::Desktop::getInstance().getDefaultLookAndFeel()
		.findColour(juce::ResizableWindow::backgroundColourId),
		juce::DocumentWindow::allButtons)
{
	formatManager.addDefaultFormats();

	setUsingNativeTitleBar(true);
	setContentOwned(new MainComponent(formatManager), true);

#if JUCE_IOS || JUCE_ANDROID
	setFullScreen(true);
#else
	setResizable(true, true);
	centreWithSize(getWidth(), getHeight());
#endif

	setTopLeftPosition(60, 60);

	knownPluginList.setCustomScanner(std::make_unique<CustomPluginScanner>());

	if (auto savedPluginList = getAppProperties().getUserSettings()->getXmlValue("pluginList"))
		knownPluginList.recreateFromXml(*savedPluginList);

    knownPluginList.addChangeListener (this);

	// ŒŸØ‚Ì‚½‚ß‚Æ‚è‚ ‚¦‚¸‚±‚±‚ÅŠJ‚­
	if (pluginListWindow == nullptr)
		pluginListWindow.reset(new PluginListWindow(*this, formatManager));
	pluginListWindow->toFront(true);


	setVisible(true);
}

MainWindow::~MainWindow()
{
	pluginListWindow = nullptr;
	knownPluginList.removeChangeListener(this);

	getAppProperties().getUserSettings()->setValue("mainWindowPos", getWindowStateAsString());
	clearContentComponent();

#if ! (JUCE_ANDROID || JUCE_IOS)
#if JUCE_MAC
	setMacMainMenu(nullptr);
#else
	setMenuBar(nullptr);
#endif
#endif
}

void MainWindow::changeListenerCallback(juce::ChangeBroadcaster* changed)
{
	if (changed == &knownPluginList)
	{
		// save the plugin list every time it gets changed, so that if we're scanning
		// and it crashes, we've still saved the previous ones
		if (auto savedPluginList = std::unique_ptr<juce::XmlElement>(knownPluginList.createXml()))
		{
			getAppProperties().getUserSettings()->setValue("pluginList", savedPluginList.get());
			getAppProperties().saveIfNeeded();
		}
	}
}

void MainWindow::tryToQuitApplication()
{
	juce::JUCEApplication::quit();
}

