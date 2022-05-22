#include "EditorWindow.h"
#include "MainComponent.h"

EditorWindow::EditorWindow(MainComponent& mw, juce::String& title, juce::AudioProcessorEditor* editor)
	: DocumentWindow(title,
		juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
		juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton),
	owner(mw)
{
	setContentOwned(editor, true);
	setResizable(editor->isResizable(), false);
	setVisible(true);
	toFront(true);
}

EditorWindow::~EditorWindow()
{
	clearContentComponent();
}

void EditorWindow::closeButtonPressed()
{
	owner.editorWindow = nullptr;
}

