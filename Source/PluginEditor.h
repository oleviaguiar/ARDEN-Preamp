#pragma once
#include "PluginProcessor.h"
class ArdenEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit ArdenEditor(ArdenProcessor&);
 ~ArdenEditor() override=default;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 ArdenProcessor& processor;
 using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
 using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
 using ComboAttachment=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
 std::array<juce::Slider,7> knobs;
 std::array<std::unique_ptr<SliderAttachment>,7> knobLinks;
 juce::ComboBox circuit,freq,meter;
 std::unique_ptr<ComboAttachment> circuitLink,freqLink,meterLink;
 juce::ToggleButton power{"POWER"},eq{"EQ IN"};
 std::unique_ptr<ButtonAttachment> powerLink,eqLink;
 float displayedVU=-40,clipHold=0;
 void timerCallback() override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArdenEditor)
};
