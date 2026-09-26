#include "PluginEditor.h"

NoirLookAndFeel::NoirLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111214));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff32353a));
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff111214));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
}

void NoirLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float sliderPos, float start, float end, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const float radius = diameter * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = start + sliderPos * (end - start);

    g.setColour (juce::Colour (0xff090a0c));
    g.fillEllipse (centre.x - radius, centre.y - radius, diameter, diameter);
    g.setColour (juce::Colour (0xff2c2f34));
    g.drawEllipse (centre.x - radius, centre.y - radius, diameter, diameter, 1.5f);

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, start, angle, true);
    g.setColour (juce::Colour (0xfff2f2f2));
    g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -radius + 10.0f, 3.0f, radius * 0.36f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (juce::Colour (0xffd5d7da));
    g.fillPath (pointer);
}

void NoirLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (b.getToggleState() ? juce::Colour (0xffeeeeee) : juce::Colour (0xff15171a));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (b.getToggleState() ? juce::Colours::black : juce::Colour (0xffc8c8c8));
    g.setFont (13.0f);
    g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

NoirSaturatorAudioProcessorEditor::NoirSaturatorAudioProcessorEditor (NoirSaturatorAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&noirLaf);
    setSize (860, 520);

    setupKnob (drive, driveLabel, "DRIVE");
    setupKnob (lowDrive, lowLabel, "LOW");
    setupKnob (midDrive, midLabel, "MID");
    setupKnob (highDrive, highLabel, "HIGH");
    setupKnob (x1, x1Label, "XOVER 1");
    setupKnob (x2, x2Label, "XOVER 2");
    setupKnob (mix, mixLabel, "MIX");
    setupKnob (output, outLabel, "OUTPUT");

    drive.setTextValueSuffix (" dB");
    lowDrive.setTextValueSuffix (" dB");
    midDrive.setTextValueSuffix (" dB");
    highDrive.setTextValueSuffix (" dB");
    x1.setTextValueSuffix (" Hz");
    x2.setTextValueSuffix (" Hz");
    mix.setTextValueSuffix (" %");
    output.setTextValueSuffix (" dB");

    style.addItemList ({ "Warm", "Tape", "Tube", "Soft Clip", "Hard Clip", "Fold" }, 1);
    addAndMakeVisible (style);
    addAndMakeVisible (hq);

    auto& apvts = processor.parameters;
    driveAtt = std::make_unique<SliderAttachment> (apvts, "drive", drive);
    lowAtt = std::make_unique<SliderAttachment> (apvts, "lowDrive", lowDrive);
    midAtt = std::make_unique<SliderAttachment> (apvts, "midDrive", midDrive);
    highAtt = std::make_unique<SliderAttachment> (apvts, "highDrive", highDrive);
    x1Att = std::make_unique<SliderAttachment> (apvts, "crossover1", x1);
    x2Att = std::make_unique<SliderAttachment> (apvts, "crossover2", x2);
    mixAtt = std::make_unique<SliderAttachment> (apvts, "mix", mix);
    outAtt = std::make_unique<SliderAttachment> (apvts, "output", output);
    styleAtt = std::make_unique<ComboAttachment> (apvts, "style", style);
    hqAtt = std::make_unique<ButtonAttachment> (apvts, "hq", hq);

    startTimerHz (30);
}

NoirSaturatorAudioProcessorEditor::~NoirSaturatorAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void NoirSaturatorAudioProcessorEditor::setupKnob (juce::Slider& s, juce::Label& l, const juce::String& text)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 20);
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffdddddd));
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0b0c0e));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff24272b));
    addAndMakeVisible (s);

    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, juce::Colour (0xffaeb2b7));
    l.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
    addAndMakeVisible (l);
}

void NoirSaturatorAudioProcessorEditor::timerCallback()
{
    inDb = processor.getInputMeterDb();
    outDb = processor.getOutputMeterDb();
    repaint();
}

void NoirSaturatorAudioProcessorEditor::drawMeter (juce::Graphics& g, juce::Rectangle<float> r, float db, const juce::String& label)
{
    g.setColour (juce::Colour (0xff0b0c0e));
    g.fillRoundedRectangle (r, 4.0f);
    const float norm = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 3.0f, 0.0f, 1.0f));
    auto fill = r.withTop (r.getBottom() - r.getHeight() * norm).reduced (2.0f);
    g.setColour (db > -1.0f ? juce::Colour (0xfff2f2f2) : juce::Colour (0xff777b81));
    g.fillRoundedRectangle (fill, 3.0f);
    g.setColour (juce::Colour (0xff8e9298));
    g.setFont (10.0f);
    g.drawText (label, r.translated (0, r.getHeight() + 4.0f).toNearestInt(), juce::Justification::centred);
}

void NoirSaturatorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff050607));

    juce::ColourGradient grad (juce::Colour (0xff15171a), 0, 0,
                               juce::Colour (0xff070809), 0, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (10.0f), 14.0f);

    g.setColour (juce::Colour (0xffeeeeee));
    g.setFont (juce::FontOptions (26.0f).withStyle ("Bold"));
    g.drawText ("WILTIZ-SATURATOR", 32, 24, 350, 40, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff747980));
    g.setFont (12.0f);
    g.drawText ("MULTIBAND HARMONIC COLOR", 34, 60, 300, 18, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff23262a));
    g.drawLine (30.0f, 92.0f, 830.0f, 92.0f, 1.0f);

    g.setColour (juce::Colour (0xff0a0b0d));
    g.fillRoundedRectangle (juce::Rectangle<float> (170, 115, 510, 228), 10.0f);
    g.setColour (juce::Colour (0xff292c31));
    g.drawRoundedRectangle (juce::Rectangle<float> (170, 115, 510, 228), 10.0f, 1.0f);

    drawMeter (g, { 42.0f, 128.0f, 16.0f, 188.0f }, inDb, "IN");
    drawMeter (g, { 802.0f, 128.0f, 16.0f, 188.0f }, outDb, "OUT");

    g.setColour (juce::Colour (0xff61666c));
    g.setFont (11.0f);
    g.drawText ("Wiltiz Audio • v1.0", 32, getHeight() - 34, 220, 18, juce::Justification::centredLeft);
}

void NoirSaturatorAudioProcessorEditor::resized()
{
    const int top = 125;
    drive.setBounds (75, top, 90, 110); driveLabel.setBounds (75, top - 18, 90, 18);
    lowDrive.setBounds (190, top + 25, 105, 110); lowLabel.setBounds (190, top + 8, 105, 18);
    midDrive.setBounds (325, top + 25, 105, 110); midLabel.setBounds (325, top + 8, 105, 18);
    highDrive.setBounds (460, top + 25, 105, 110); highLabel.setBounds (460, top + 8, 105, 18);
    output.setBounds (690, top, 90, 110); outLabel.setBounds (690, top - 18, 90, 18);

    x1.setBounds (190, 245, 105, 92); x1Label.setBounds (190, 228, 105, 18);
    x2.setBounds (460, 245, 105, 92); x2Label.setBounds (460, 228, 105, 18);

    style.setBounds (300, 365, 180, 32);
    hq.setBounds (495, 365, 86, 32);
    mix.setBounds (610, 354, 100, 104); mixLabel.setBounds (610, 338, 100, 18);
    drive.setBounds (75, 145, 90, 110);
}
