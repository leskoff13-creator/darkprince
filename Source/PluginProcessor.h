#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PitchShifter.h"

class DarkPrinceProcessor : public juce::AudioProcessor
{
public:
    DarkPrinceProcessor();
    ~DarkPrinceProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dark Prince"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }

    int getNumPrograms() override { return 3; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> outLevel { 0.0f };   // для анимации в редакторе

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Duplicator = juce::dsp::ProcessorDuplicator<Filter, Coeffs>;

    void updateEq (float dark);

    static constexpr int maxCh = 2;
    PitchShifter mainShifter[maxCh], subShifter[maxCh];
    DryDelay dryDelay[maxCh];
    float subLp1[maxCh] {}, subLp2[maxCh] {};
    float subLpCoef = 0.05f;

    juce::dsp::ProcessorChain<Duplicator, Duplicator, Duplicator, Duplicator, Duplicator> eq;
    juce::dsp::Reverb reverb;
    juce::dsp::Limiter<float> limiter;

    juce::SmoothedValue<float> pitchSm, mixSm, outSm;
    std::vector<float> ratioBuf, mixBuf, outBuf, dryBuf[maxCh];

    std::atomic<float>* pPitch = nullptr;
    std::atomic<float>* pSub   = nullptr;
    std::atomic<float>* pDark  = nullptr;
    std::atomic<float>* pGrit  = nullptr;
    std::atomic<float>* pSpace = nullptr;
    std::atomic<float>* pMix   = nullptr;
    std::atomic<float>* pOut   = nullptr;

    double sr = 44100.0;
    float lastDark = -1.0f;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkPrinceProcessor)
};
