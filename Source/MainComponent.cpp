#include <Windows.h>
#include "MainComponent.h"
#include "EditorWindow.h"
#include "MainWindow.h"
#include "MyAudioPlayHead.h"

void* edit(void* component) {
	((MainComponent*)component)->edit();
	return nullptr;
}

void* quit(void* component) {
	((MainComponent*)component)->quit();
	return nullptr;
}

void* openPluginListWindow(void* component) {
	((MainComponent*)component)->openPluginListWindow();
	return nullptr;
}

void* getState(void* component) {
	((MainComponent*)component)->getState();
	return nullptr;
}

void* setState(void* component) {
	((MainComponent*)component)->setState();
	return nullptr;
}

const byte COMMAND_INSTRUMENT = 1;
const byte COMMAND_EFFECT = 2;
const byte COMMAND_MANAGE = 3;
const byte COMMAND_EDIT = 4;
const byte COMMAND_QUIT = 5;
const byte COMMAND_GET_STATE = 6;
const byte COMMAND_SET_STATE = 7;

void proc(MainComponent* component) {
	auto plugin = component->plugin.get();

	MyAudioPlayHead audioPlayHead;
	plugin->setPlayHead(&audioPlayHead);

	std::string path("\\\\.\\pipe\\pluin-host");
	path += std::to_string(_getpid());
	HANDLE hPipe;
	for (int i = 0; i < 10; ++i) {
		hPipe = CreateFile(path.c_str(),
			(GENERIC_READ | GENERIC_WRITE),
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hPipe != INVALID_HANDLE_VALUE) {
			break;
		}
		juce::Logger::getCurrentLogger()->writeToLog(juce::String(juce::CharPointer_UTF8("パイプ開けません")));
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	if (INVALID_HANDLE_VALUE != hPipe) {
		const int framesPerBuffer = 1024;
		byte buffer[framesPerBuffer * 10];
		WORD readLength = 0;
		WORD writeLength = 0;
		juce::MidiBuffer midiBuffer;
		const int midiEventSize = 6;
		// TODO 適切なチャンネルサイズ
		juce::AudioBuffer<float> audioBuffer(16, framesPerBuffer);

		byte playing;
		double bpm;
		juce::int64 timeInSamples;

		auto loop = true;
		while (loop && hPipe != INVALID_HANDLE_VALUE) {
			// ブロックする
			ReadFile(hPipe, buffer, 1, (LPDWORD)&readLength, nullptr);
			if (readLength == 0) {
				continue;
			}
			auto command = buffer[0];

			if (command == COMMAND_INSTRUMENT || command == COMMAND_EFFECT) {
				ReadFile(hPipe, &playing, 1, (LPDWORD)&readLength, nullptr);
				ReadFile(hPipe, &bpm, 8, (LPDWORD)&readLength, nullptr);
				ReadFile(hPipe, &timeInSamples, 8, (LPDWORD)&readLength, nullptr);
				audioPlayHead.isPlaying = (playing != 0);
				audioPlayHead.bpm = bpm;
				audioPlayHead.timeInSamples = timeInSamples;
			}
			switch (command) {
			case COMMAND_INSTRUMENT: {
				midiBuffer.clear();
				ReadFile(hPipe, buffer, 2, (LPDWORD)&readLength, nullptr);
				int len = buffer[1] * 0x100 + buffer[0];
				if (len > 0) {
					ReadFile(hPipe, buffer, len * midiEventSize, (LPDWORD)&readLength, nullptr);
					for (int i = 0; i < len; ++i) {
						byte event = buffer[i * midiEventSize];
						int channel = buffer[i * midiEventSize + 1];
						int note = buffer[i * midiEventSize + 2];
						float velocity = buffer[i * midiEventSize + 3] / 127.0f;
						int frame = buffer[i * midiEventSize + 5] * 0x100 + buffer[i * midiEventSize + 4];
						switch (event) {
						case 0x90:
							midiBuffer.addEvent(juce::MidiMessage::noteOn(channel, note, velocity), frame);
							break;
						case 0x80:
							midiBuffer.addEvent(juce::MidiMessage::noteOff(channel, note, velocity), frame);
							break;
						case 0xB0:
							if (note == 0x7B) {
								midiBuffer.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
							}
						}
					}
				}
				audioBuffer.clear();
				plugin->processBlock(audioBuffer, midiBuffer);
				WriteFile(hPipe, audioBuffer.getReadPointer(0), framesPerBuffer * 4, (LPDWORD)&writeLength, nullptr);
				WriteFile(hPipe, audioBuffer.getReadPointer(1), framesPerBuffer * 4, (LPDWORD)&writeLength, nullptr);

				audioPlayHead.timeInSamples += framesPerBuffer;
				break;
			}
			case COMMAND_EFFECT: {
				midiBuffer.clear();
				audioBuffer.clear();
				// TODO 指定したバイト読むまでループとかした方がいい
				ReadFile(hPipe, audioBuffer.getWritePointer(0), framesPerBuffer * 4, (LPDWORD)&readLength, nullptr);
				DBG("ReadFile L " << std::to_string(readLength));
				ReadFile(hPipe, audioBuffer.getWritePointer(1), framesPerBuffer * 4, (LPDWORD)&readLength, nullptr);
				DBG("ReadFile R " << std::to_string(readLength));
				plugin->processBlock(audioBuffer, midiBuffer);
				DBG(audioBuffer.getSample(0, 0));
				DBG(audioBuffer.getSample(1, 0));
				WriteFile(hPipe, audioBuffer.getReadPointer(0), framesPerBuffer * 4, (LPDWORD)&writeLength, nullptr);
				WriteFile(hPipe, audioBuffer.getReadPointer(1), framesPerBuffer * 4, (LPDWORD)&writeLength, nullptr);

				audioPlayHead.timeInSamples += framesPerBuffer;
				break;
			}
			case COMMAND_MANAGE: {
				juce::MessageManager::getInstance()->callFunctionOnMessageThread(openPluginListWindow, component);
			}
			case COMMAND_EDIT: {
				juce::MessageManager::getInstance()->callAsync([component]() {
					juce::MessageManager::getInstance()->callFunctionOnMessageThread(edit, component);
					});
				break;
			}
			case COMMAND_QUIT: {
				loop = false;
				break;
			}
			case COMMAND_GET_STATE: {
				component->hPipe = hPipe;
				juce::MessageManager::getInstance()->callFunctionOnMessageThread(getState, component);
				{
					DBG("latency " << plugin->getLatencySamples());
				}
				break;
			}
			case COMMAND_SET_STATE: {
				component->hPipe = hPipe;
				juce::MessageManager::getInstance()->callFunctionOnMessageThread(setState, component);
				break;
			}
			}
		}
		CloseHandle(hPipe);
	}
	else {
		DBG("Can not create file, as PIPE\r\n");
	}
	juce::MessageManager::getInstance()->callFunctionOnMessageThread(quit, component);
}

//==============================================================================
MainComponent::MainComponent(
	MainWindow& mw,
	juce::String& pn,
	juce::AudioPluginFormatManager& fm,
	juce::KnownPluginList& kpl
) : owner(mw), formatManager(fm), knownPluginList(kpl)
{
	setSize(400, 300);
	pluginName_ = pn.trimCharactersAtStart("\"").trimCharactersAtEnd("\"");

	addAndMakeVisible(checkTheTimeButton);
	checkTheTimeButton.setButtonText("Check the time...");
	checkTheTimeButton.onClick = [this] { click(); };

	addAndMakeVisible(timeLabel);
	timeLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black);
	timeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
	timeLabel.setJustificationType(juce::Justification::centred);

	if (!pluginName_.isEmpty()) {
		auto types = knownPluginList.getTypes();
		auto desc = std::find_if(types.begin(), types.end(), [this](auto desc) {
			return desc.name == pluginName_; });
		if (desc != types.end()) {
			std::cout << desc->descriptiveName << std::endl;
			juce::String errorMessage;
			plugin = formatManager.createPluginInstance(*desc, 48000, 1024,
				errorMessage);
			DBG("after createPluginInstance " << " :[" << errorMessage << "]");
			juce::Logger::getCurrentLogger()->writeToLog(errorMessage);
			plugin->enableAllBuses();
			plugin->prepareToPlay(48000, 1024);

			std::thread t(proc, this);
			t.detach();
		}
		else
		{
			juce::Logger::getCurrentLogger()->writeToLog(juce::String(juce::CharPointer_UTF8("プラグインがない ")) + pluginName_);
		}
	}
}

MainComponent::~MainComponent()
{
	editorWindow = nullptr;
	if (plugin) {
		plugin->releaseResources();
		plugin.reset();
	}
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	g.setFont(juce::Font(16.0f));
	g.setColour(juce::Colours::white);
	g.drawText("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
	// This is called when the MainComponent is resized.
	// If you add any child components, this is where you should
	// update their positions.
	checkTheTimeButton.setBounds(10, 10, getWidth() - 20, 40);
	timeLabel.setBounds(10, 60, getWidth() - 20, 40);

}

void MainComponent::click()
{
	DBG("clicked!");
	edit();
}

void MainComponent::edit() {
	if (editorWindow != nullptr) {
		return;
	}
	if (!plugin->hasEditor()) {
		DBG("not hasEditor");
		return;
	}
	auto editor = plugin->createEditor();
	if (editor == nullptr) {
		DBG("createEditor NG");
		return;
	}

	if (editorWindow == nullptr) {
		editorWindow.reset(new EditorWindow(*this, pluginName_, editor));
	}
}

void MainComponent::openPluginListWindow() {
	owner.openPluginListWindow();
}

void MainComponent::quit() {
	owner.tryToQuitApplication();
}

void MainComponent::getState() {
	byte buffer[4];
	DWORD writeLength;
	juce::MemoryBlock mb;
	plugin->getStateInformation(mb);
	buffer[0] = static_cast<byte>(mb.getSize() & 0xff);
	buffer[1] = static_cast<byte>((mb.getSize() >> 8) & 0xff);
	buffer[2] = static_cast<byte>((mb.getSize() >> 16) & 0xff);
	buffer[3] = static_cast<byte>((mb.getSize() >> 24) & 0xff);
	WriteFile(hPipe, buffer, 4, (LPDWORD)&writeLength, nullptr);
	WriteFile(hPipe, mb.getData(), (DWORD)mb.getSize(), (LPDWORD)&writeLength, nullptr);
}

void MainComponent::setState() {
	byte buffer[4];
	DWORD readLength;
	ReadFile(hPipe, buffer, 4, (LPDWORD)&readLength, nullptr);
	int len = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + buffer[0];
	juce::MemoryBlock mb(len);
	ReadFile(hPipe, mb.getData(), (DWORD)mb.getSize(), (LPDWORD)&readLength, nullptr);
	plugin->setStateInformation(mb.getData(), (int)mb.getSize());
}

