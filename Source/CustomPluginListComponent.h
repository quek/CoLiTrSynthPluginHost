#pragma once
#include <JuceHeader.h>

constexpr const char* scanModeKey = "pluginScanMode";

class CustomPluginListComponent : public juce::PluginListComponent
{
public:
	CustomPluginListComponent(juce::AudioPluginFormatManager& manager,
		juce::KnownPluginList& listToRepresent,
		const juce::File& pedal,
		juce::PropertiesFile* props,
		bool async);

	void resized() override
	{
		PluginListComponent::resized();

		const auto& buttonBounds = getOptionsButton().getBounds();
		validationModeBox.setBounds(buttonBounds.withWidth(130).withRightX(getWidth() - buttonBounds.getX()));
	}

private:
	juce::Label validationModeLabel{ {}, "Scan mode" };
	juce::ComboBox validationModeBox;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomPluginListComponent)
};
