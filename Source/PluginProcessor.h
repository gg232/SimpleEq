/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#define low_cut_freq_string "LowCut Freq"
#define low_cut_slope_string "LowCut Slope"
#define N_low_cut_freq_default_value 20.f
#define N_low_cut_freq_skewFactor 1.f

#define high_cut_freq_string "HighCut Freq"
#define high_cut_slope_string "HighCut Slope"
#define N_high_cut_freq_default_value 20000.f
#define N_high_cut_freq_skewFactor 1.f

#define PK_freq_string "Peak Freq"
#define PK_gain_string "Peak Gain"
#define PK_Q_string "Peak Q"
#define N_PK_freq_default_value 750.f
#define N_PK_freq_SkewFactor 1.f

using APVTS = juce::AudioProcessorValueTreeState;

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float peakFreq{ 0.f },
        peakGainInDecibels{ 0.f },
        peakQuality{ 1.f },
        lowCutFreq{ 0.f },
        highCutFreq{ 0.f };
    Slope lowCutSlope{ Slope_12 }, highCutSlope{ Slope_12 };

};

ChainSettings getChainSettings(APVTS& apvts);

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static APVTS::ParameterLayout createParameterLayout();
    //static b/c it doesn't use any member variables
    //juce::UndoManager undo_manager = juce::UndoManager(30000,30);
    APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    MonoChain leftChain, rightChain;
    
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    
    void updatePeakFilter(const ChainSettings& chainSettings);
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, 
                const CoefficientType& coefficients)
    {
        updateCoefficients(
            chain.template get<Index>().coefficients, 
            coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& leftLowCut,
        const CoefficientType& cutCoefficients,
        const Slope& lowCutSlope)
    {
        /*
        Supposedly we "need" to call these with keyword "template", but I compiled it w/out and it worked just fine.
        Let's leave it in unless it causes problems down the road
        */

        leftLowCut.template setBypassed<0>(true);
        leftLowCut.template setBypassed<1>(true);
        leftLowCut.template setBypassed<2>(true);
        leftLowCut.template setBypassed<3>(true);

        switch (lowCutSlope)
        {
            case Slope_48:
            {
                update<3>(leftLowCut, cutCoefficients);
                /*
                *leftLowCut.template get<3>().coefficients
                    = *cutCoefficients[3];
                leftLowCut.template setBypassed<3>(false);
                */
            }

            case Slope_36:
            {
                
                update<2>(leftLowCut, cutCoefficients);
                /*
                *leftLowCut.template get<3>().coefficients
                    = *cutCoefficients[3];
                leftLowCut.template setBypassed<3>(false);
                */
            }

            case Slope_24:
            {
                update<1>(leftLowCut, cutCoefficients);
                /*
                *leftLowCut.template get<3>().coefficients
                    = *cutCoefficients[3];
                leftLowCut.template setBypassed<3>(false);
                */
            }

            case Slope_12:
            {
                update<0>(leftLowCut, cutCoefficients);
            }
        }
    }

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainsettings);

    void updateFilters();
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
