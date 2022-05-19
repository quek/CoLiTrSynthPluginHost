/*
  ==============================================================================

	EditorComponent.cpp
	Created: 19 May 2022 7:16:04pm
	Author:  ancient

  ==============================================================================
*/

#include <JuceHeader.h>
#include "EditorComponent.h"

//==============================================================================
EditorComponent::EditorComponent(juce::AudioProcessorEditor* edt)
{
	editor.reset(edt);
	addAndMakeVisible(*editor);
	resized();
}

EditorComponent::~EditorComponent()
{
	removeChildComponent(editor.get());
	editor.reset();
}

void EditorComponent::resized()
{
	setBounds(editor->getLocalBounds());
}
