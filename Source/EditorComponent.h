/*
  ==============================================================================

	EditorComponent.h
	Created: 19 May 2022 7:16:04pm
	Author:  ancient

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class EditorComponent : public juce::Component
{
public:
	EditorComponent(juce::AudioProcessorEditor*);
	~EditorComponent() override;

	void resized() override;

private:
	std::unique_ptr<juce::AudioProcessorEditor> editor;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorComponent)
};
