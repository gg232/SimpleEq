/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/




#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>

auto low_cut_freq_parameter_ID = "LowCut freq",
     low_cut_freq_parameter_name = "LowCut freq";
float low_cut_freq_default_value = 20.f,
      low_cut_freq_skewFactor = 1.f;

auto high_cut_freq_parameter_ID = "HighCut Freq",
high_cut_freq_parameter_name = "HighCut Freq";
float high_cut_freq_default_value = 20000.f,
high_cut_freq_skewFactor = 1.f;

auto PK_freq_parameter_ID = "Peak Freq",
PK_freq_parameter_name = "Peak Freq";
float PK_freq_default_value = 750.f,
PK_freq_skewFactor = 1.f;

auto PK_gain_parameter_ID = "Peak Gain",
PK_gain_parameter_name = "Peak Gain";

auto PK_Q_parameter_ID = "Peak Q",
PK_Q_parameter_name = "Peak Q";

auto low_cut_slope_parameter_ID = "LowCut Slope",
low_cut_slope_parameter_name = "LowCut Slope";

auto high_cut_slope_parameter_ID = "HighCut Slope",
high_cut_slope_parameter_name = "HighCut Slope";


//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    //come up with specs for each channel/chain
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    //prepare each chain (Left and Right)
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto chainSettings = getChainSettings(apvts);

    
    using dB = juce::Decibels;
    using coeffs = juce::dsp::IIR::Coefficients<float>;

    /*
    Reference-counted wrapper around an array
    Allocated on the heap (personal note: not optimal, better to allocate on stack!)
    */
    updatePeakFilter(chainSettings);

    auto cutCoefficients = 
        juce::dsp::FilterDesign<float>
        ::designIIRHighpassHighOrderButterworthMethod
        (chainSettings.lowCutFreq, sampleRate,
        (chainSettings.lowCutSlope+1)*2);
    
    //Left chain
    auto& leftLowCut = leftChain.get
        <ChainPositions::LowCut>();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
        case Slope_48:
        {
            *leftLowCut.get<3>().coefficients 
                = *cutCoefficients[3];
            leftLowCut.setBypassed<3>(false);
        }

        case Slope_36:
        {
            *leftLowCut.get<2>().coefficients =
                *cutCoefficients[2];
            leftLowCut.setBypassed<2>(false);
        }

        case Slope_24:
        {
            *leftLowCut.get<1>().coefficients =
                *cutCoefficients[1];
            leftLowCut.setBypassed<1>(false);
        }

        case Slope_12:
        {
            *leftLowCut.get<0>().coefficients =
                *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
        }
    }

    //right chain
    auto& rightLowCut = rightChain.get
        <ChainPositions::LowCut>();

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
        case Slope_48:
        {
            *rightLowCut.get<3>().coefficients
                = *cutCoefficients[3];
            rightLowCut.setBypassed<3>(false);
        }

        case Slope_36:
        {
            *rightLowCut.get<2>().coefficients =
                *cutCoefficients[2];
            rightLowCut.setBypassed<2>(false);
        }

        case Slope_24:
        {
            *rightLowCut.get<1>().coefficients =
                *cutCoefficients[1];
            rightLowCut.setBypassed<1>(false);
        }

        case Slope_12:
        {
            *rightLowCut.get<0>().coefficients =
                *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
        }
    }


    
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    //Always do audio processing before updating settings!
    
    auto chainSettings = getChainSettings(apvts);

    using dB = juce::Decibels;
    using coeffs = juce::dsp::IIR::Coefficients<float>;

    /*
    Reference-counted wrapper around an array
    Allocated on the heap (?!?)
    */
    updatePeakFilter(chainSettings);
    

    auto cutCoefficients =
        juce::dsp::FilterDesign<float>
        ::designIIRHighpassHighOrderButterworthMethod
        (chainSettings.lowCutFreq, getSampleRate(),
            (chainSettings.lowCutSlope + 1) * 2);

    //Left chain
    auto& leftLowCut = leftChain.get
        <ChainPositions::LowCut>();

    updateCutFilter(leftLowCut,
        cutCoefficients,
        chainSettings);

    /*leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
        case Slope_48:
        {
            *leftLowCut.get<3>().coefficients
                = *cutCoefficients[3];
            leftLowCut.setBypassed<3>(false);
        }

        case Slope_36:
        {
            *leftLowCut.get<2>().coefficients =
                *cutCoefficients[2];
            leftLowCut.setBypassed<2>(false);
        }

        case Slope_24:
        {
            *leftLowCut.get<1>().coefficients =
                *cutCoefficients[1];
            leftLowCut.setBypassed<1>(false);
        }

        case Slope_12:
        {
            *leftLowCut.get<0>().coefficients =
                *cutCoefficients[0];
            leftLowCut.setBypassed<0>(false);
        }
    }*/

    //right chain
    auto& rightLowCut = rightChain.get
        <ChainPositions::LowCut>();

    updateCutFilter(rightLowCut,
        cutCoefficients,
        chainSettings);

    /*
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
        case Slope_48:
        {
            *rightLowCut.get<3>().coefficients
                = *cutCoefficients[3];
            rightLowCut.setBypassed<3>(false);
        }

        case Slope_36:
        {
            *rightLowCut.get<2>().coefficients =
                *cutCoefficients[2];
            rightLowCut.setBypassed<2>(false);
        }

        case Slope_24:
        {
            *rightLowCut.get<1>().coefficients =
                *cutCoefficients[1];
            rightLowCut.setBypassed<1>(false);
        }

        case Slope_12:
        {
            *rightLowCut.get<0>().coefficients =
                *cutCoefficients[0];
            rightLowCut.setBypassed<0>(false);
        }
    }
    */

    juce::dsp::AudioBlock<float> block(buffer);
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    //Now we can create processing contexts that wrap each individual audio block
    using J_process_context = juce::dsp::ProcessContextReplacing<float>;
    J_process_context leftContext(leftBlock),
                      rightContext(rightBlock);

    //Pass contexts to mono filter chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    //retugetChainSettingsrn new SimpleEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

/*
This function adds a generic float knob to the GUI.
*/
void add_knob(const char* knob_ID, const char* knob_name, float knob_default_value,
                   float range_min, float range_max, float range_spacing, float knob_skewFactor, APVTS::ParameterLayout &layout)
{
    using namespace juce;

    NormalisableRange<float> normalisable_range =
        NormalisableRange<float>(range_min, range_max, range_spacing, knob_skewFactor);
    //layout.add(std::make_unique<AudioParameterFloat>(knob_ID, knob_name, range_min, range_max, knob_default_value));
        
    layout.add(std::make_unique<AudioParameterFloat>
        (knob_ID, knob_name,
            normalisable_range, knob_default_value));
            
}

void SimpleEQAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

APVTS::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;

    //using namespace juce;
    using J_range = juce::NormalisableRange<float>;
    using J_float = juce::AudioParameterFloat;
    

    /*
    
    J_range normalisable_range =
        J_range(20.f,20000.f,1.f,LP_freq_skewFactor);
    */

    /*
    layout.add(std::make_unique<J_float>
        (LP_freq_parameter_ID, LP_freq_parameter_name,
         normalisable_range, LP_freq_default_value));
    
    */

    /* //add_knob() usage:
    add_knob(const char* knob_ID, const char* knob_name, 
            float knob_default_value, float range_min,
            float range_max, float range_spacing,
            float knob_skewFactor, APVTS::ParameterLayout& layout)
    */

    /*
    Note: with generic UI, the order the buttons are added below is the order you'll see.
    They are not necessarily the order these filters are processed in.
    */

    //Low cut
    add_knob(low_cut_freq_parameter_ID, low_cut_freq_parameter_name,
        low_cut_freq_default_value, 20.f,
        20000.f, 1.f,
        0.25f, layout);

    //High cut
    add_knob(high_cut_freq_parameter_ID, high_cut_freq_parameter_name,
            high_cut_freq_default_value, 20.f,
            20000.f, 1.f,
            0.25f, layout);

    //Peak
    add_knob(PK_freq_parameter_ID, PK_freq_parameter_name,
        PK_freq_default_value, 20.0f,
        20000.f, 1.f,
        0.25f, layout);

    /* //add_knob() usage:
    add_knob(const char* knob_ID, const char* knob_name, 
            float knob_default_value, float range_min,
            float range_max, float range_spacing,
            float knob_skewFactor, APVTS::ParameterLayout& layout)
    */
    
    //Peak gain
    add_knob(PK_gain_parameter_ID, PK_gain_parameter_name,
        0.f, -24.f,
        24.f, 0.05f,
        1.f, layout);

    /* //add_knob() usage:
    add_knob(const char* knob_ID, const char* knob_name,
            float knob_default_value, float range_min,
            float range_max, float range_spacing,
            float knob_skewFactor, APVTS::ParameterLayout& layout)
    */

    //Peak Q
    add_knob(PK_Q_parameter_ID, PK_Q_parameter_name,
        1.0, 0.1f,
        10.f, 0.05f,
        1.f, layout);

    using J_StringArray = juce::StringArray;
    using J_String = juce::String;
    using J_choice = juce::AudioParameterChoice;

    J_StringArray HP_LP_slope_string;

    for(int i = 0; i < 4; i++)
    {
        J_String str;
        str << (i + 1) * 12;
        str << " dB/Octave";
        HP_LP_slope_string.add(str);
    }

    int default_slope = 0;

    layout.add(std::make_unique<J_choice>
        (low_cut_slope_parameter_ID, low_cut_slope_parameter_name, HP_LP_slope_string, default_slope));
    layout.add(std::make_unique<J_choice>
        (high_cut_slope_parameter_ID, high_cut_slope_parameter_name, HP_LP_slope_string, default_slope));


    return layout;
}
/*
//Helper fxn in ChainSettings to get settings for each parameter
auto get_param(APVTS& apvts, char* param_ID)
{
    return apvts.getRawParameterValue(param_ID)->load();
}*/

/*

This function gets parameters from the (internal) APVTS
into my ChainSettings struct.

*/
ChainSettings getChainSettings(APVTS& apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue(low_cut_freq_parameter_ID)->load();
    settings.highCutFreq = apvts.getRawParameterValue(high_cut_freq_parameter_ID)->load();
    settings.peakFreq = apvts.getRawParameterValue(PK_freq_parameter_ID)->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue(PK_gain_parameter_ID)->load();
    settings.peakQuality = apvts.getRawParameterValue(PK_Q_parameter_ID)->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue(low_cut_slope_parameter_ID)->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue(high_cut_slope_parameter_ID)->load());
    /*
    //This code breaks the program for some reason.
    settings.lowCutFreq = get_param(apvts, "LowCut Freq");
    settings.highCutFreq = get_param(apvts, "HighCut");
    settings.peakFreq = get_param(apvts, "Peak Freq");
    settings.peakGainInDecibels = get_param(apvts, "Peak Gain");
    settings.peakQuality = get_param(apvts, "Peak Q");
    settings.lowCutSlope = (int)(get_param(apvts, "LowCut Slope"));
    settings.highCutSlope = (int)(get_param(apvts, "HighCut"));
    */

    return settings;
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    using dB = juce::Decibels;
    using coeffs = juce::dsp::IIR::Coefficients<float>;
    auto peakCoefficients = coeffs::makePeakFilter(
        getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality,
        dB::decibelsToGain(chainSettings.peakGainInDecibels));

    /*
    *leftChain.get<ChainPositions::Peak>().coefficients
        = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients
        = *peakCoefficients;
        */

    updateCoefficients(
        leftChain.get<ChainPositions::Peak>().coefficients,
        peakCoefficients);
    updateCoefficients(
        rightChain.get<ChainPositions::Peak>().coefficients,
        peakCoefficients);

}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
