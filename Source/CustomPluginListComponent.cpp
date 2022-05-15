#include "CustomPluginListComponent.h"
#include "Main.h"

CustomPluginListComponent::CustomPluginListComponent(juce::AudioPluginFormatManager& manager,
	juce::KnownPluginList& listToRepresent,
	const juce::File& pedal,
	juce::PropertiesFile* props,
	bool async)
	: PluginListComponent(manager, listToRepresent, pedal, props, async)
{
	addAndMakeVisible(validationModeLabel);
	addAndMakeVisible(validationModeBox);

	validationModeLabel.attachToComponent(&validationModeBox, true);
	validationModeLabel.setJustificationType(juce::Justification::right);
	validationModeLabel.setSize(100, 30);

	auto unusedId = 1;

	for (const auto mode : { "In-process", "Out-of-process" })
		validationModeBox.addItem(mode, unusedId++);

	validationModeBox.setSelectedItemIndex(getAppProperties().getUserSettings()->getIntValue(scanModeKey));

	validationModeBox.onChange = [this]
	{
		getAppProperties().getUserSettings()->setValue(scanModeKey, validationModeBox.getSelectedItemIndex());
	};

	resized();
}
