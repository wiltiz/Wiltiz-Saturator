#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace IDs
{
    constexpr auto drive = "drive";
    constexpr auto lowDrive = "lowDrive";
    constexpr auto midDrive = "midDrive";
    constexpr auto highDrive = "highDrive";
    constexpr auto crossover1 = "crossover1";
    constexpr auto crossover2 = "crossover2";
    constexpr auto style = "style";
    constexpr auto mix = "mix";
    constexpr auto output = "output";
    constexpr auto hq = "hq";
}

NoirSaturatorAudioProcessor::NoirSaturatorAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "STATE", createParameterLayout())
{
}

NoirSaturatorAudioProcessor::APVTS::ParameterLayout NoirSaturatorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::drive, "Drive", juce::NormalisableRange<float> (-6.0f, 30.0f, 0.1f), 6.0f, "dB"));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::lowDrive, "Low Drive", juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::midDrive, "Mid Drive", juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::highDrive, "High Drive", juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::crossover1, "Low/Mid Crossover", juce::NormalisableRange<float> (60.0f, 1200.0f, 1.0f, 0.35f), 250.0f, "Hz"));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::crossover2, "Mid/High Crossover", juce::NormalisableRange<float> (1200.0f, 12000.0f, 1.0f, 0.35f), 4200.0f, "Hz"));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::style, "Style", juce::StringArray { "Warm", "Tape", "Tube", "Soft Clip", "Hard Clip", "Fold" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::mix, "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, "%"));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::output, "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, "dB"));
    p.push_back (std::make_unique<juce::AudioParameterBool> (IDs::hq, "HQ 2x", true));
    return { p.begin(), p.end() };
}

void NoirSaturatorAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& f : lowSplit)  f.reset();
    for (auto& f : highSplit) f.reset();
    prevLowIn.fill (0.0f);
    prevMidIn.fill (0.0f);
    prevHighIn.fill (0.0f);
}

bool NoirSaturatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

float NoirSaturatorAudioProcessor::saturateSample (float x, int style)
{
    switch (style)
    {
        case 0: // Warm - smooth odd harmonics
            return std::tanh (x);
        case 1: // Tape - soft compression plus slight asymmetry
        {
            const float biased = x + 0.08f;
            const float y = std::tanh (1.15f * biased) - std::tanh (0.092f);
            return 0.94f * y;
        }
        case 2: // Tube - asymmetrical transfer, richer even harmonics
        {
            const float pos = std::tanh (1.35f * x);
            const float even = 0.16f * (x * x) / (1.0f + std::abs (x));
            return juce::jlimit (-1.25f, 1.25f, pos + (x >= 0.0f ? even : -0.45f * even));
        }
        case 3: // Soft clip
        {
            const float ax = std::abs (x);
            if (ax <= 1.0f) return x - (x * x * x) / 3.0f;
            return std::copysign (2.0f / 3.0f, x);
        }
        case 4: // Hard clip
            return juce::jlimit (-1.0f, 1.0f, x);
        case 5: // Fold - restrained wavefolding
        {
            float y = x;
            if (y > 1.0f || y < -1.0f)
            {
                y = std::fmod (y + 1.0f, 4.0f);
                if (y < 0.0f) y += 4.0f;
                y = (y <= 2.0f) ? (y - 1.0f) : (3.0f - y);
            }
            return y;
        }
        default: return std::tanh (x);
    }
}

float NoirSaturatorAudioProcessor::saturateHQ (float x, float& previousInput, int style)
{
    // Lightweight 2x oversampling approximation: evaluate a midpoint and current sample,
    // then average. This lowers high-frequency distortion products without a heavy FIR stage.
    const float mid = 0.5f * (previousInput + x);
    const float y0 = saturateSample (mid, style);
    const float y1 = saturateSample (x, style);
    previousInput = x;
    return 0.5f * (y0 + y1);
}

void NoirSaturatorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numChannels = juce::jmin (2, buffer.getNumChannels());
    const auto numSamples = buffer.getNumSamples();

    const float driveDb = parameters.getRawParameterValue (IDs::drive)->load();
    const float lowDb   = driveDb + parameters.getRawParameterValue (IDs::lowDrive)->load();
    const float midDb   = driveDb + parameters.getRawParameterValue (IDs::midDrive)->load();
    const float highDb  = driveDb + parameters.getRawParameterValue (IDs::highDrive)->load();
    const float lowGain = dbToGain (lowDb);
    const float midGain = dbToGain (midDb);
    const float highGain = dbToGain (highDb);
    const float x1 = parameters.getRawParameterValue (IDs::crossover1)->load();
    float x2 = parameters.getRawParameterValue (IDs::crossover2)->load();
    x2 = juce::jmax (x1 + 100.0f, x2);
    const int style = (int) parameters.getRawParameterValue (IDs::style)->load();
    const float wet = 0.01f * parameters.getRawParameterValue (IDs::mix)->load();
    const float outGain = dbToGain (parameters.getRawParameterValue (IDs::output)->load());
    const bool hq = parameters.getRawParameterValue (IDs::hq)->load() >= 0.5f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        lowSplit[(size_t) ch].setCutoff (x1, currentSampleRate);
        highSplit[(size_t) ch].setCutoff (x2, currentSampleRate);
    }

    float inPeak = 0.0f, outPeak = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& lowFilter = lowSplit[(size_t) ch];
        auto& upperFilter = highSplit[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = data[i];
            inPeak = juce::jmax (inPeak, std::abs (dry));

            // Complementary split: low + mid + high exactly reconstruct the input before saturation.
            const float low = lowFilter.process (dry);
            const float upper = dry - low;
            const float mid = upperFilter.process (upper);
            const float high = upper - mid;

            float l = low * lowGain;
            float m = mid * midGain;
            float h = high * highGain;

            if (hq)
            {
                l = saturateHQ (l, prevLowIn[(size_t) ch], style);
                m = saturateHQ (m, prevMidIn[(size_t) ch], style);
                h = saturateHQ (h, prevHighIn[(size_t) ch], style);
            }
            else
            {
                l = saturateSample (l, style);
                m = saturateSample (m, style);
                h = saturateSample (h, style);
            }

            const float saturated = l + m + h;
            const float y = (dry + wet * (saturated - dry)) * outGain;
            data[i] = y;
            outPeak = juce::jmax (outPeak, std::abs (y));
        }
    }

    const float newInDb = juce::Decibels::gainToDecibels (juce::jmax (inPeak, 1.0e-5f));
    const float newOutDb = juce::Decibels::gainToDecibels (juce::jmax (outPeak, 1.0e-5f));
    inputDb.store (0.82f * inputDb.load() + 0.18f * newInDb);
    outputDb.store (0.82f * outputDb.load() + 0.18f * newOutDb);
}

void NoirSaturatorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NoirSaturatorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* NoirSaturatorAudioProcessor::createEditor()
{
    return new NoirSaturatorAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoirSaturatorAudioProcessor();
}
