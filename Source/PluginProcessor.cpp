/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProtectYourEars.h"

//==============================================================================
DelayAudioProcessor::DelayAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
	params(apvts)
{
    //do nothing
}

DelayAudioProcessor::~DelayAudioProcessor()
{
}

//==============================================================================
const juce::String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
	params.prepareToPlay(sampleRate);
	params.reset();

	juce::dsp::ProcessSpec spec;
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = juce::uint32(samplesPerBlock);
    spec.numChannels = 2;
	delayLine.prepare(spec);

	double numSamples = (Parameters::maxDelayTime / 1000.0) * sampleRate;
	int maxDelayInSamples = int(std::ceil(numSamples));
	delayLine.setMaximumDelayInSamples(maxDelayInSamples);
	delayLine.reset();

	feedbackL = 0.0f;
	feedbackR = 0.0f;
}

void DelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  //#if JucePlugin_IsMidiEffect
  //  juce::ignoreUnused (layouts);
  //  return true;
  //#else
  //  // This is the place where you check if the layout is supported.
  //  // In this template code we only support mono or stereo.
  //  // Some plugin hosts, such as certain GarageBand versions, will only
  //  // load plugins that support stereo bus layouts.
  //  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
  //   && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
  //      return false;

  //  // This checks if the input layout matches the output layout
  // #if ! JucePlugin_IsSynth
  //  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
  //      return false;
  // #endif

  //  return true;
  //#endif

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void DelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, [[maybe_unused]] juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	params.update(); // Update the parameters from the APVTS

    float sampleRate = float(getSampleRate());

	float* channelDataL = buffer.getWritePointer(0);
	float* channelDataR = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
		params.smoothen(); // Smoothen the parameters for this sample

        float delayInSamples = (params.delayTime / 1000.0f) * sampleRate;
        delayLine.setDelay(delayInSamples);

        float dryL = channelDataL[sample];
		float dryR = channelDataR[sample];

        //float mono = (dryL + dryL) * 0.5f;

		delayLine.pushSample(0, dryL+feedbackL);
		delayLine.pushSample(1, dryR+feedbackR);

		float wetL = delayLine.popSample(0);
		float wetR = delayLine.popSample(1);

		feedbackL = wetL * params.feedback;
		feedbackR = wetR * params.feedback;

        float mixL = dryL + wetL * params.mix;
		float mixR = dryR + wetR * params.mix;

        channelDataL[sample] = mixL * params.gain;
		channelDataR[sample] = mixR * params.gain;
    }

#if JUCE_DEBUG
	protectYourEars(buffer);
#endif
}

//==============================================================================
bool DelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor (*this);
}

//==============================================================================
void DelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
	copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void DelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
	std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
	if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())){
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}

