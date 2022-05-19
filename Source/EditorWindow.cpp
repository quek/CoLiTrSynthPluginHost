#include "EditorWindow.h"
#include "MainComponent.h"
#include "EditorComponent.h"

EditorWindow::EditorWindow(MainComponent& mw, juce::String title, juce::AudioProcessorEditor* editor)
	: DocumentWindow(title,
		juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
		juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton),
	owner(mw)
{
	setContentOwned(new EditorComponent(editor), true);

	setResizable(true, false);
	setResizeLimits(300, 400, 800, 1500);
	setTopLeftPosition(60, 60);

	setVisible(true);
}

EditorWindow::~EditorWindow()
{
	clearContentComponent();
}

void EditorWindow::closeButtonPressed()
{
	owner.editorWindow = nullptr;
}

