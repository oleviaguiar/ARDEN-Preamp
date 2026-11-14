#pragma once
#include "PluginProcessor.h"
#include <array>

class ArdenEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ArdenEditor(ArdenProcessor&);
    ~ArdenEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct TransparentKnobLook : juce::LookAndFeel_V4
    {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float,
                              float, float, juce::Slider&) override;
    } knobLook;
    ArdenProcessor& processor;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::array<juce::Slider, 6> knobs;
    std::array<std::unique_ptr<SliderAttachment>, 6> knobLinks;
    juce::ComboBox circuit, freq, meter;
    std::unique_ptr<ComboAttachment> circuitLink, freqLink, meterLink;
    juce::ToggleButton power, eq;
    std::unique_ptr<ButtonAttachment> powerLink, eqLink;
    juce::Image panel;
    float displayedVU = -40.0f, clipHold = 0.0f;
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArdenEditor)
};
