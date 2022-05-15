#include <Windows.h>
#include "MainComponent.h"

void* edit(void* component) {
	((MainComponent*)component)->edit();
	return nullptr;
}

void proc(MainComponent* component) {
	HANDLE hPipe = CreateFile("\\\\.\\pipe\\pluin-host",
		(GENERIC_READ | GENERIC_WRITE),
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == hPipe) {
		DBG("Can not create file, as PIPE\r\n");
	}

	byte buffer[10];
	WORD len = 0;

	for (int i = 0; i < 1000; ++i) {
		// ブロックする
		ReadFile(hPipe, buffer, 10, (LPDWORD)&len, nullptr);
		DBG("ReadFile " << buffer[0] << " len " << std::to_string(len));
		if (len == 0) {
			continue;
		}
		auto command = buffer[0];
		switch (command) {
		case 1:
			juce::MessageManager::getInstance()->callFunctionOnMessageThread(edit, component);
			break;
		}
		buffer[0] = 7;
		buffer[1] = 3;
		// ブロックする
		WriteFile(hPipe, buffer, 10, (LPDWORD)&len, nullptr);
		DBG("WriteFile " << std::to_string(len));
		ReadFile(hPipe, buffer, 10, (LPDWORD)&len, nullptr);
		DBG("ReadFile " << buffer[0] << " len " << std::to_string(len));
	}
	CloseHandle(hPipe);
}

//==============================================================================
MainComponent::MainComponent()
{
	addAndMakeVisible(checkTheTimeButton);
	checkTheTimeButton.setButtonText("Check the time...");
	checkTheTimeButton.onClick = [this] { click(); };

	addAndMakeVisible(timeLabel);
	timeLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black);
	timeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
	timeLabel.setJustificationType(juce::Justification::centred);

	setSize(600, 400);

	formatManager.addDefaultFormats();

	std::cout << knownPluginList.getNumTypes() << std::endl;
	// std::unique_ptr<juce::PluginDescription, std::default_delete<juce::PluginDescription>> desc = knownPluginList.getTypeForFile("C:\\Program Files\\Common Files\\VST3\\Vital.vst3");
	//std::cout << desc << std::endl;

	auto files = juce::StringArray();
	files.add("C:\\Program Files\\Common Files\\VST3\\Vital.vst3");
	files.add("C:\\Program Files\\Common Files\\VST3\\MeldaProduction\\MSoundFactory.vst3");
	files.add("C:\\Program Files\\Common Files\\VST3\\Dexed.vst3");
	juce::OwnedArray<juce::PluginDescription> typesFound;
	knownPluginList.scanAndAddDragAndDroppedFiles(formatManager, files, typesFound);
	std::cout << knownPluginList.getNumTypes() << " " << typesFound.size() << std::endl;
	auto desc = knownPluginList.getTypes()[0];
	std::cout << desc.descriptiveName << std::endl;


	juce::String errorMessage;

	plugin = formatManager.createPluginInstance(desc, 48000, 1024,
		errorMessage);
	DBG("after createPluginInstance " << " :[" << errorMessage << "]");
	plugin->prepareToPlay(48000, 1024);

	std::thread t(proc, this);
	t.detach();

}

MainComponent::~MainComponent()
{
	if (editor) {
		removeChildComponent(editor.get());
		editor.reset();
	}
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

	if (editor) {
		setBounds(editor->getLocalBounds());
	}
}

void MainComponent::click()
{
	DBG("clicked!");

}

void MainComponent::edit() {
	if (!plugin->hasEditor()) {
		DBG("not hasEditor");
		return;
	}
	auto x = plugin->createEditor();
	if (x == nullptr) {
		DBG("createEditor NG");
		return;
	}
	else {
		DBG("createEditor OK");
	}
	editor.reset(x);
	addAndMakeVisible(*editor);
	resized();
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

