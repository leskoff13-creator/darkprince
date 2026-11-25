#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float mainWindowMs = 50.0f;
    constexpr float subWindowMs  = 70.0f;

    struct Preset { const char* name; float pitch, sub, dark, grit, space, mix, out; };
    const Preset presets[] = {
        { "Тёмный принц", -4.0f, 35.0f, 70.0f, 30.0f, 25.0f, 100.0f,  0.0f },
        { "Шёпот тьмы",   -2.0f, 10.0f, 85.0f, 10.0f, 45.0f, 100.0f,  0.0f },
        { "Бездна",       -8.0f, 60.0f, 60.0f, 55.0f, 40.0f, 100.0f, -2.0f },
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout DarkPrinceProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<P> (juce::ParameterID { "pitch", 1 }, "Pitch",     R (-12.0f, 12.0f, 0.01f), -4.0f));
    l.add (std::make_unique<P> (juce::ParameterID { "sub",   1 }, "Sub",       R (0.0f, 100.0f, 0.1f),   35.0f));
    l.add (std::make_unique<P> (juce::ParameterID { "dark",  1 }, "Darkness",  R (0.0f, 100.0f, 0.1f),   70.0f));
    l.add (std::make_unique<P> (juce::ParameterID { "grit",  1 }, "Grit",      R (0.0f, 100.0f, 0.1f),   30.0f));
    l.add (std::make_unique<P> (juce::ParameterID { "space", 1 }, "Space",     R (0.0f, 100.0f, 0.1f),   25.0f));
    l.add (std::make_unique<P> (juce::ParameterID { "mix",   1 }, "Mix",       R (0.0f, 100.0f, 0.1f),  100.0f));
    l.add (std::make_unique<P> (juce::ParameterID { "out",   1 }, "Output",    R (-12.0f, 12.0f, 0.1f),   0.0f));
    return l;
}

DarkPrinceProcessor::DarkPrinceProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pPitch = apvts.getRawParameterValue ("pitch");
    pSub   = apvts.getRawParameterValue ("sub");
    pDark  = apvts.getRawParameterValue ("dark");
    pGrit  = apvts.getRawParameterValue ("grit");
    pSpace = apvts.getRawParameterValue ("space");
    pMix   = apvts.getRawParameterValue ("mix");
    pOut   = apvts.getRawParameterValue ("out");
}

bool DarkPrinceProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in  = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void DarkPrinceProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    for (int c = 0; c < maxCh; ++c)
    {
        mainShifter[c].prepare (sr, mainWindowMs);
        subShifter[c].prepare (sr, subWindowMs);
        dryDelay[c].prepare ((int) mainShifter[c].getLatencySamples());
        subLp1[c] = subLp2[c] = 0.0f;
        dryBuf[c].assign ((size_t) samplesPerBlock, 0.0f);
    }
    setLatencySamples ((int) mainShifter[0].getLatencySamples());

    // ~450 Гц ФНЧ для саб-слоя
    subLpCoef = 1.0f - std::exp (-6.2831853f * 450.0f / (float) sr);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };
    eq.prepare (spec);
    reverb.prepare (spec);
    limiter.prepare (spec);
    limiter.setThreshold (-1.0f);
    limiter.setRelease (80.0f);

    *eq.get<0>().state = *Coeffs::makeHighPass (sr, 60.0f, 0.707f);
    lastDark = -1.0f;
    updateEq (*pDark * 0.01f);

    pitchSm.reset (sr, 0.05);  pitchSm.setCurrentAndTargetValue (*pPitch);
    mixSm.reset   (sr, 0.03);  mixSm.setCurrentAndTargetValue   (*pMix * 0.01f);
    outSm.reset   (sr, 0.03);  outSm.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain ((float) *pOut));

    ratioBuf.assign ((size_t) samplesPerBlock, 1.0f);
    mixBuf.assign   ((size_t) samplesPerBlock, 1.0f);
    outBuf.assign   ((size_t) samplesPerBlock, 1.0f);
}

// Кривая EQ подобрана по спектру референса: основная энергия 80–300 Гц,
// выше ~300 Гц спад примерно 5 дБ на октаву, выше 5 кГц почти ничего.
void DarkPrinceProcessor::updateEq (float dark)
{
    if (std::abs (dark - lastDark) < 1.0e-4f) return;
    lastDark = dark;

    *eq.get<1>().state = *Coeffs::makeLowShelf  (sr, 250.0f,  0.7f, juce::Decibels::decibelsToGain (5.0f * dark));
    *eq.get<2>().state = *Coeffs::makeHighShelf (sr, 1500.0f, 0.6f, juce::Decibels::decibelsToGain (-10.0f * dark));
    *eq.get<3>().state = *Coeffs::makePeakFilter (sr, 3000.0f, 0.8f, juce::Decibels::decibelsToGain (-4.0f * dark));
    const float lp = juce::jmin (juce::jmap (dark, 12000.0f, 5500.0f), (float) sr * 0.45f);
    *eq.get<4>().state = *Coeffs::makeLowPass (sr, lp, 0.707f);
}

void DarkPrinceProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin (buffer.getNumChannels(), maxCh);
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    if ((int) ratioBuf.size() < n)
    {
        ratioBuf.resize ((size_t) n); mixBuf.resize ((size_t) n); outBuf.resize ((size_t) n);
        for (auto& d : dryBuf) d.resize ((size_t) n);
    }
    for (auto& d : dryBuf) if ((int) d.size() < n) d.resize ((size_t) n);

    const float subAmt = *pSub * 0.01f * 0.9f;
    const float grit   = *pGrit * 0.01f;
    const float space  = *pSpace * 0.01f;
    const float drive  = 1.0f + 7.0f * grit;
    const float driveNorm = 1.0f / std::tanh (drive);
    const float pitchSt = (float) *pPitch;

    pitchSm.setTargetValue (pitchSt);
    mixSm.setTargetValue (*pMix * 0.01f);
    outSm.setTargetValue (juce::Decibels::decibelsToGain ((float) *pOut));
    updateEq (*pDark * 0.01f);

    for (int i = 0; i < n; ++i)
    {
        ratioBuf[(size_t) i] = pitchSm.getNextValue();
        mixBuf[(size_t) i]   = mixSm.getNextValue();
        outBuf[(size_t) i]   = outSm.getNextValue();
    }

    // 1) питч + саб-слой (октавой ниже) + сатурация
    for (int c = 0; c < numCh; ++c)
    {
        auto* d = buffer.getWritePointer (c);
        auto* dry = dryBuf[c].data();

        for (int i = 0; i < n; ++i)
        {
            const float in = d[i];
            const float ratio    = std::pow (2.0f, ratioBuf[(size_t) i] / 12.0f);
            const float subRatio = ratio * 0.5f;

            float wet = mainShifter[c].process (in, ratio);

            float sub = subShifter[c].process (in, subRatio);
            subLp1[c] += subLpCoef * (sub - subLp1[c]);
            subLp2[c] += subLpCoef * (subLp1[c] - subLp2[c]);
            wet += subLp2[c] * subAmt * 2.0f;

            wet = std::tanh (wet * drive) * driveNorm;

            dry[i] = dryDelay[c].process (in);
            d[i] = wet;
        }
    }

    // 2) тёмный EQ
    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);
    eq.process (ctx);

    // 3) тёмный реверб
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize   = 0.5f + 0.4f * space;
    rp.damping    = 0.85f;
    rp.wetLevel   = 0.12f * space;
    rp.dryLevel   = 0.5f;
    rp.width      = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.process (ctx);

    // 4) dry/wet, громкость, лимитер
    float peak = 0.0f;
    for (int c = 0; c < numCh; ++c)
    {
        auto* d = buffer.getWritePointer (c);
        const auto* dry = dryBuf[c].data();
        for (int i = 0; i < n; ++i)
        {
            const float m = mixBuf[(size_t) i];
            d[i] = (dry[i] * (1.0f - m) + d[i] * m) * outBuf[(size_t) i];
            peak = juce::jmax (peak, std::abs (d[i]));
        }
    }
    limiter.process (ctx);

    const float prev = outLevel.load();
    outLevel.store (peak > prev ? peak : prev * 0.9f);
}

void DarkPrinceProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, getNumPrograms() - 1, index);
    currentProgram = index;
    const auto& p = presets[index];
    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    set ("pitch", p.pitch); set ("sub", p.sub);   set ("dark", p.dark); set ("grit", p.grit);
    set ("space", p.space); set ("mix", p.mix);   set ("out", p.out);
}

const juce::String DarkPrinceProcessor::getProgramName (int index)
{
    return juce::String::fromUTF8 (presets[juce::jlimit (0, 2, index)].name);
}

void DarkPrinceProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void DarkPrinceProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* DarkPrinceProcessor::createEditor() { return new DarkPrinceEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DarkPrinceProcessor(); }
