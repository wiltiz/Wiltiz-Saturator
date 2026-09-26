#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NoirLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NoirLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class NoirSaturatorAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoirSaturatorAudioProcessorEditor (NoirSaturatorAudioProcessor&);
    ~NoirSaturatorAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    NoirSaturatorAudioProcessor& processor;
    NoirLookAndFeel noirLaf;

    juce::Slider drive, lowDrive, midDrive, highDrive, x1, x2, mix, output;
    juce::ComboBox style;
    juce::ToggleButton hq { "HQ 2x" };
    juce::Label driveLabel, lowLabel, midLabel, highLabel, x1Label, x2Label, mixLabel, outLabel;

    std::unique_ptr<SliderAttachment> driveAtt, lowAtt, midAtt, highAtt, x1Att, x2Att, mixAtt, outAtt;
    std::unique_ptr<ComboAttachment> styleAtt;
    std::unique_ptr<ButtonAttachment> hqAtt;

    float inDb = -100.0f, outDb = -100.0f;
    void timerCallback() override;
    void setupKnob (juce::Slider&, juce::Label&, const juce::String&);
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, float db, const juce::String& label);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirSaturatorAudioProcessorEditor)
};
