/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayAudioProcessorEditor::DelayAudioProcessorEditor (DelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), meter(p.levelL, p.levelR)
{
	setLookAndFeel(&mainLF);

	tempoSyncButton.setButtonText("Sync");
	tempoSyncButton.setClickingTogglesState(true);
	tempoSyncButton.setBounds(0, 0, 70, 27);
	tempoSyncButton.setLookAndFeel(ButtonLookAndFeel::get());
	delayGroup.addAndMakeVisible(tempoSyncButton);

	delayGroup.setText("Delay");
	delayGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
	delayGroup.addAndMakeVisible(delayTimeKnob);
	delayGroup.addChildComponent(delayNoteKnob);
	addAndMakeVisible(delayGroup);

	feedbackGroup.setText("Feedback");
	feedbackGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
	feedbackGroup.addAndMakeVisible(feedbackKnob);
	feedbackGroup.addAndMakeVisible(stereoKnob);
	feedbackGroup.addAndMakeVisible(lowCutKnob);
	feedbackGroup.addAndMakeVisible(highCutKnob);
	addAndMakeVisible(feedbackGroup);

	outputGroup.setText("Output");
	outputGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
	outputGroup.addAndMakeVisible(gainKnob);
	outputGroup.addAndMakeVisible(mixKnob);
	outputGroup.addAndMakeVisible(meter);
	addAndMakeVisible(outputGroup);

	updateDelayKnobs(audioProcessor.params.tempoSyncParam->get());
	audioProcessor.params.tempoSyncParam->addListener(this);


    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (500, 330);
}

DelayAudioProcessorEditor::~DelayAudioProcessorEditor()
{
	audioProcessor.params.tempoSyncParam->removeListener(this);
	setLookAndFeel(nullptr);
}

//==============================================================================
void DelayAudioProcessorEditor::paint (juce::Graphics& g)
{
	auto noise = juce::ImageCache::getFromMemory(BinaryData::Noise_png, BinaryData::Noise_pngSize);
	auto fillType = juce::FillType(noise, juce::AffineTransform::scale(0.5f));
	g.setFillType(fillType);
	g.fillRect(getLocalBounds());

	auto rect = getLocalBounds().withHeight(40);
	g.setColour(Colors::header);
	g.fillRect(rect);

	auto image = juce::ImageCache::getFromMemory(BinaryData::Logo_png, BinaryData::Logo_pngSize);

	int destWidth = image.getWidth() / 2;
	int destHeight = image.getHeight() / 2;
	g.drawImage(image, getWidth() / 2 - destWidth / 2, 0, destWidth, destHeight, 0, 0, image.getWidth(), image.getHeight());
}

void DelayAudioProcessorEditor::resized()
{
	auto bounds = getLocalBounds();
	int y = 50;
	int height = bounds.getHeight() - 60;

	delayGroup.setBounds(10, y, 110, height);

	outputGroup.setBounds(bounds.getWidth() - 160, y, 150, height);

	feedbackGroup.setBounds(delayGroup.getRight() + 10, y, outputGroup.getX()-delayGroup.getRight() - 20, height);
	feedbackKnob.setTopLeftPosition(20, 20);

	delayTimeKnob.setTopLeftPosition(20, 20);
	mixKnob.setTopLeftPosition(20, 20);
	gainKnob.setTopLeftPosition(mixKnob.getX(), mixKnob.getBottom()+10);
	tempoSyncButton.setTopLeftPosition(20, delayTimeKnob.getBottom() + 10);
	delayNoteKnob.setTopLeftPosition(delayTimeKnob.getX(), delayTimeKnob.getY());

	stereoKnob.setTopLeftPosition(feedbackKnob.getRight()+20, 20);

	lowCutKnob.setTopLeftPosition(feedbackKnob.getX(), stereoKnob.getBottom() + 10);
	highCutKnob.setTopLeftPosition(lowCutKnob.getRight()+20, lowCutKnob.getY());

	meter.setBounds(outputGroup.getWidth() - 45, 30, 30, gainKnob.getBottom() - 30);
}

void DelayAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
//	DBG("Parameter " << parameterIndex << " changed to " << newValue);
	if (juce::MessageManager::getInstance()->isThisTheMessageThread())
		updateDelayKnobs(newValue!=0.0f);
	else
		juce::MessageManager::callAsync([this, newValue] { updateDelayKnobs(newValue !=0.0f); });
}

void DelayAudioProcessorEditor::updateDelayKnobs(bool tempoSyncActivate)
{
	delayTimeKnob.setVisible(!tempoSyncActivate);
	delayNoteKnob.setVisible(tempoSyncActivate);
}
