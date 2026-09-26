#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class NoirSaturatorAudioProcessor : public juce::AudioProcessor
{
public:
    NoirSaturatorAudioProcessor();
    ~NoirSaturatorAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

    float getInputMeterDb() const noexcept { return inputDb.load(); }
    float getOutputMeterDb() const noexcept { return outputDb.load(); }

private:
    struct CascadeLP
    {
        std::array<float, 4> z { 0, 0, 0, 0 };
        float a = 0.0f;

        void reset() { z.fill (0.0f); }
        void setCutoff (float hz, double sr)
        {
            const auto f = juce::jlimit (10.0f, (float) (0.45 * sr), hz);
            a = std::exp (-juce::MathConstants<float>::twoPi * f / (float) sr);
        }
        float process (float x)
        {
            for (auto& s : z)
            {
                s = (1.0f - a) * x + a * s;
                x = s;
            }
            return x;
        }
    };

    std::array<CascadeLP, 2> lowSplit;
    std::array<CascadeLP, 2> highSplit;
    double currentSampleRate = 44100.0;
    std::array<float, 2> prevLowIn { 0, 0 }, prevMidIn { 0, 0 }, prevHighIn { 0, 0 };

    std::atomic<float> inputDb { -100.0f };
    std::atomic<float> outputDb { -100.0f };

    static float saturateSample (float x, int style);
    static float saturateHQ (float x, float& previousInput, int style);
    static float dbToGain (float db) noexcept { return juce::Decibels::decibelsToGain (db); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirSaturatorAudioProcessor)
};
