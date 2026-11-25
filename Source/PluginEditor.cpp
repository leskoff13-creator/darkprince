#include "PluginEditor.h"

using juce::Colour;

PrinceLookAndFeel::PrinceLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId,     Colour (0xffd9cfe8));
    setColour (juce::Slider::textBoxOutlineColourId,  juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId,             Colour (0xffb9a6d6));
}

void PrinceLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const juce::PathStrokeType stroke (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour (Colour (0xff2a2236));
    g.strokePath (track, stroke);

    juce::Path val;
    val.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);
    g.setGradientFill (juce::ColourGradient (Colour (0xff7b2cff), c.x - r, c.y + r,
                                             Colour (0xffff2d55), c.x + r, c.y - r, false));
    g.strokePath (val, stroke);

    const float rb = r - 9.0f;
    g.setGradientFill (juce::ColourGradient (Colour (0xff3a2f4d), c.x, c.y - rb,
                                             Colour (0xff120d1a), c.x, c.y + rb, false));
    g.fillEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f);
    g.setColour (Colour (0xff4b3d66));
    g.drawEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f, 1.0f);

    juce::Path p;
    p.addRoundedRectangle (-1.5f, -rb + 3.0f, 3.0f, rb * 0.45f, 1.5f);
    g.setColour (Colour (0xffffc4d6));
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

DarkPrinceEditor::DarkPrinceEditor (DarkPrinceProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (660, 360);

    addKnob (kPitch, juce::String::fromUTF8 ("ПИТЧ"),  "pitch", " st", 1);
    addKnob (kSub,   juce::String::fromUTF8 ("САБ"),   "sub",   " %",   0);
    addKnob (kDark,  juce::String::fromUTF8 ("МРАК"),  "dark",  " %",   0);
    addKnob (kGrit,  juce::String::fromUTF8 ("ГРЯЗЬ"), "grit",  " %",   0);
    addKnob (kSpace, juce::String::fromUTF8 ("ЭХО"),   "space", " %",   0);
    addKnob (kMix,   juce::String::fromUTF8 ("МИКС"),  "mix",   " %",   0);
    addKnob (kOut,   juce::String::fromUTF8 ("ВЫХОД"), "out",   " dB",  1);

    startTimerHz (30);
}

DarkPrinceEditor::~DarkPrinceEditor()
{
    setLookAndFeel (nullptr);
}

void DarkPrinceEditor::addKnob (Knob& k, const juce::String& text, const char* id,
                                const juce::String& suffix, int decimals)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    k.slider.setTextValueSuffix (suffix);
    k.slider.setNumDecimalPlacesToDisplay (decimals);
    k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (k.label);

    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void DarkPrinceEditor::resized()
{
    auto area = getLocalBounds().reduced (24, 0);
    area.removeFromTop (140);
    area.removeFromBottom (20);
    const int colW = area.getWidth() / 7;

    for (auto* k : knobs)
    {
        auto col = area.removeFromLeft (colW);
        k->label.setBounds (col.removeFromTop (20));
        k->slider.setBounds (col);
    }
}

void DarkPrinceEditor::timerCallback()
{
    const float level = proc.outLevel.load();
    glow += (juce::jlimit (0.0f, 1.0f, level * 2.5f) - glow) * 0.25f;

    const int spawn = (int) (glow * 3.0f) + (rng.nextFloat() < 0.15f ? 1 : 0);
    for (int i = 0; i < spawn && embers.size() < 120; ++i)
        embers.push_back ({ rng.nextFloat() * (float) getWidth(), (float) getHeight() + 4.0f,
                            0.6f + rng.nextFloat() * 1.8f, (rng.nextFloat() - 0.5f) * 0.6f,
                            1.0f + rng.nextFloat() * 2.5f, 1.0f });

    for (auto& e : embers)
    {
        e.y += -e.vy;
        e.x += e.drift;
        e.life -= 0.007f + e.size * 0.001f;
    }
    embers.erase (std::remove_if (embers.begin(), embers.end(),
                                  [] (const Ember& e) { return e.life <= 0.0f; }), embers.end());
    repaint();
}

void DarkPrinceEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (Colour (0xff0a0610), 0.0f, 0.0f,
                                             Colour (0xff1c0b2e), 0.0f, bounds.getHeight(), false));
    g.fillAll();

    // кроваво-фиолетовое свечение снизу, реагирует на уровень
    g.setGradientFill (juce::ColourGradient (Colour (0xffff2d55).withAlpha (0.08f + 0.30f * glow),
                                             bounds.getCentreX(), bounds.getBottom() + 40.0f,
                                             Colour (0x00000000), bounds.getCentreX(), bounds.getBottom() - 220.0f, true));
    g.fillAll();

    for (const auto& e : embers)
    {
        g.setColour (Colour (0xffff5a7a).withAlpha (e.life * 0.8f));
        g.fillEllipse (e.x, e.y, e.size, e.size);
    }

    // корона
    juce::Path crown;
    crown.startNewSubPath (0.0f, 1.0f);
    crown.lineTo (0.0f, 0.25f);  crown.lineTo (0.25f, 0.6f);
    crown.lineTo (0.5f, 0.0f);   crown.lineTo (0.75f, 0.6f);
    crown.lineTo (1.0f, 0.25f);  crown.lineTo (1.0f, 1.0f);
    crown.closeSubPath();
    const float cw = 64.0f, ch = 44.0f, cx = bounds.getCentreX() - cw * 0.5f, cy = 26.0f;
    g.setGradientFill (juce::ColourGradient (Colour (0xffff2d55), cx, cy, Colour (0xff7b2cff), cx + cw, cy + ch, false));
    g.fillPath (crown, juce::AffineTransform::scale (cw, ch).translated (cx, cy));

    g.setColour (Colour (0xfff2e8ff));
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText (juce::String::fromUTF8 ("ТЁМНЫЙ ПРИНЦ"), getLocalBounds().withTrimmedTop (76).removeFromTop (44),
                juce::Justification::centred);

    g.setColour (Colour (0xff8a74ad));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (juce::String::fromUTF8 ("голосовой преобразователь"), getLocalBounds().withTrimmedTop (114).removeFromTop (18),
                juce::Justification::centred);
}
