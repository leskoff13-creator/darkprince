#pragma once
#include "PluginProcessor.h"

class PrinceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PrinceLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
};

class DarkPrinceEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DarkPrinceEditor (DarkPrinceProcessor&);
    ~DarkPrinceEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    void addKnob (Knob& k, const juce::String& text, const char* paramId, const juce::String& suffix, int decimals);

    struct Ember { float x, y, vy, drift, size, life; };

    DarkPrinceProcessor& proc;
    PrinceLookAndFeel laf;
    Knob kPitch, kSub, kDark, kGrit, kSpace, kMix, kOut;
    Knob* knobs[7] { &kPitch, &kSub, &kDark, &kGrit, &kSpace, &kMix, &kOut };

    std::vector<Ember> embers;
    juce::Random rng;
    float glow = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkPrinceEditor)
};
