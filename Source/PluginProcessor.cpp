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
	lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
	highCutFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
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

	lowCutFilter.prepare(spec);
	lowCutFilter.reset();

	highCutFilter.prepare(spec);
	highCutFilter.reset();

	lastLowCut = lastHighCut = -1.0f;

	tempo.reset();
}

void DelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
	const auto mono = juce::AudioChannelSet::mono();
	const auto stereo = juce::AudioChannelSet::stereo();
	const auto mainIn = layouts.getMainInputChannelSet();
	const auto mainOut = layouts.getMainOutputChannelSet();

	DBG("Buses: in: " << mainIn.getDescription() << ",out: " <<mainOut.getDescription());

	if (mainIn == mono && mainOut == mono) return true;
	if (mainIn == mono && mainOut == stereo) return true;
	if (mainIn == stereo && mainOut == stereo) return true;
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

	tempo.update(getPlayHead()); // Update the tempo information from the host
	float syncedTime = float(tempo.getMillisecondsForNoteLength(params.delayNote));
	if (syncedTime > Parameters::maxDelayTime)
		syncedTime = Parameters::maxDelayTime;

    float sampleRate = float(getSampleRate());

	auto mainInput = getBusBuffer(buffer, true, 0);
	auto mainInputChannels = mainInput.getNumChannels();
    auto isMainInputStereo = mainInputChannels > 1;
	const float* inputDataL = mainInput.getReadPointer(0);
	const float* inputDataR = mainInput.getReadPointer(isMainInputStereo ? 1 : 0); // If mono, read from the same channel

	auto mainOutput = getBusBuffer(buffer, false, 0);
	auto mainOutputChannels = mainOutput.getNumChannels();
	auto isMainOutputStereo = mainOutputChannels > 1;
	float* outputDataL = mainOutput.getWritePointer(0);
	float* outputDataR = mainOutput.getWritePointer(isMainOutputStereo ? 1 : 0); // If mono, write to the same channel

    if (isMainInputStereo) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            params.smoothen(); // Smoothen the parameters for this sample

			float delayTime = params.tempoSync ? syncedTime : params.delayTime;
            float delayInSamples = (delayTime / 1000.0f) * sampleRate;
            delayLine.setDelay(delayInSamples);

            if (params.lowCut != lastLowCut) {
                lowCutFilter.setCutoffFrequency(params.lowCut);
                lastLowCut = params.lowCut;
            }

			if (params.highCut != lastHighCut) {
				highCutFilter.setCutoffFrequency(params.highCut);
				lastHighCut = params.highCut;
			}

            float dryL = inputDataL[sample];
            float dryR = inputDataR[sample];

            float mono = (dryL + dryL) * 0.5f;

            //ping-pong delay: left channel feeds right and right channel feeds left
            delayLine.pushSample(0, mono * params.panL + feedbackR);
            delayLine.pushSample(1, mono * params.panR + feedbackL);

            float wetL = delayLine.popSample(0);
            float wetR = delayLine.popSample(1);

            feedbackL = wetL * params.feedback;
			feedbackL = lowCutFilter.processSample(0,feedbackL);
			feedbackL = highCutFilter.processSample(0, feedbackL);

            feedbackR = wetR * params.feedback;
			feedbackR = lowCutFilter.processSample(1, feedbackR);
			feedbackR = highCutFilter.processSample(1, feedbackR);

            float mixL = dryL + wetL * params.mix;
            float mixR = dryR + wetR * params.mix;

            outputDataL[sample] = mixL * params.gain;
            outputDataR[sample] = mixR * params.gain;
        }

    }
    else {
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
			params.smoothen(); // Smoothen the parameters for this sample

            float delayTime = params.tempoSync ? syncedTime : params.delayTime;
            float delayInSamples = (delayTime / 1000.0f) * sampleRate;
            delayLine.setDelay(delayInSamples);

			float dry = inputDataL[sample];
			delayLine.pushSample(0, dry +feedbackL);

            float wet = delayLine.popSample(0);
			feedbackL = wet * params.feedback;
	
            float mix = dry + wet * params.mix;
			outputDataL[sample] = mix * params.gain;
		}
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

