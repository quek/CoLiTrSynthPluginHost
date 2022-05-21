#include <Windows.h>
#include "MainComponent.h"
#include "EditorWindow.h"
#include "MainWindow.h"

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

const byte COMMAND_INSTRUMENT = 1;
const byte COMMAND_EFFECT = 2;
const byte COMMAND_MANAGE = 3;
const byte COMMAND_EDIT = 4;
const byte COMMAND_QUIT = 5;

void proc(MainComponent* component) {
	auto plugin = component->plugin.get();
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
		byte buffer[1024 * 10];
		WORD readLength = 0;
		WORD writeLength = 0;
		juce::MidiBuffer midiBuffer;
		const int midiEventSize = 6;
		// TODO 適切なチャンネルサイズ
		juce::AudioBuffer<float> audioBuffer(16, 1024);

		auto loop = true;
		while (loop && hPipe != INVALID_HANDLE_VALUE) {
			// ブロックする
			ReadFile(hPipe, buffer, 1, (LPDWORD)&readLength, nullptr);
			if (readLength == 0) {
				continue;
			}
			auto command = buffer[0];
			DBG("command " << buffer[0]);
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
						}
					}
				}
				audioBuffer.clear();
				plugin->processBlock(audioBuffer, midiBuffer);
				DBG(audioBuffer.getSample(0, 0));
				DBG(audioBuffer.getSample(1, 0));
				WriteFile(hPipe, audioBuffer.getReadPointer(0), 1024 * 4, (LPDWORD)&writeLength, nullptr);
				WriteFile(hPipe, audioBuffer.getReadPointer(1), 1024 * 4, (LPDWORD)&writeLength, nullptr);
				break;
			}
			case COMMAND_EFFECT: {
				midiBuffer.clear();
				audioBuffer.clear();
				// TODO 指定したバイト読むまでループとかした方がいい
				ReadFile(hPipe, audioBuffer.getWritePointer(0), 1024 * 4, (LPDWORD)&readLength, nullptr);
				DBG("ReadFile L " << std::to_string(readLength));
				ReadFile(hPipe, audioBuffer.getWritePointer(1), 1024 * 4, (LPDWORD)&readLength, nullptr);
				DBG("ReadFile R " << std::to_string(readLength));
				plugin->processBlock(audioBuffer, midiBuffer);
				DBG(audioBuffer.getSample(0, 0));
				DBG(audioBuffer.getSample(1, 0));
				WriteFile(hPipe, audioBuffer.getReadPointer(0), 1024 * 4, (LPDWORD)&writeLength, nullptr);
				WriteFile(hPipe, audioBuffer.getReadPointer(1), 1024 * 4, (LPDWORD)&writeLength, nullptr);
				break;
			}
			case COMMAND_MANAGE: {
				juce::MessageManager::getInstance()->callFunctionOnMessageThread(openPluginListWindow, component);
			}
			case COMMAND_EDIT: {
				juce::MessageManager::getInstance()->callFunctionOnMessageThread(edit, component);
				break;
			}
			case COMMAND_QUIT: {
				loop = false;
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
	auto pluginName = pn.trimCharactersAtStart("\"").trimCharactersAtEnd("\"");

	addAndMakeVisible(checkTheTimeButton);
	checkTheTimeButton.setButtonText("Check the time...");
	checkTheTimeButton.onClick = [this] { click(); };

	addAndMakeVisible(timeLabel);
	timeLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black);
	timeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
	timeLabel.setJustificationType(juce::Justification::centred);

	if (!pluginName.isEmpty()) {
		auto types = knownPluginList.getTypes();
		auto desc = std::find_if(types.begin(), types.end(), [pluginName](auto desc) {
			return desc.name == pluginName; });
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
			juce::Logger::getCurrentLogger()->writeToLog(juce::String("プラグインがない ") + pluginName);
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
		editorWindow.reset(new EditorWindow(*this, "TODO title", editor));
	}
	editorWindow->toFront(true);
}

void MainComponent::openPluginListWindow() {
	owner.openPluginListWindow();
}

void MainComponent::quit() {
	owner.tryToQuitApplication();
}

void MainComponent::play() {
	juce::MidiBuffer mb_;
	auto last_pitch_ = 0;
	auto sample_pos_ = 0;
	std::unique_ptr<juce::AudioBuffer<float>> buffer(new juce::AudioBuffer<float>(2, 1024));
	auto get_note_at = [](auto sample_rate, auto sample_pos) {
		static std::vector<int> const pitches = { 60, 62, 64, 65, 67, 69, 71, 72 };
		return pitches[((int)((sample_pos / sample_rate) * 2)) % pitches.size()];
	};
	for (int i = 0; i < 1; ++i) {
		// 指定したサンプル位置でのノートを返す。

		auto const pitch = get_note_at(48000, sample_pos_);

		if (pitch != last_pitch_) {
			if (last_pitch_ != -1) { mb_.addEvent(juce::MidiMessage::noteOff(1, last_pitch_, 0.5f), 0); }
			mb_.addEvent(juce::MidiMessage::noteOn(1, pitch, 0.5f), 0);
			last_pitch_ = pitch;
		}

		buffer->clear();

		// プラグインのフレーム処理を実行
		plugin->processBlock(*buffer, mb_);
		DBG("buffer[0]" << buffer->getSample(0, 0));

		mb_.clear();
		sample_pos_ += 1024;
	}
}
