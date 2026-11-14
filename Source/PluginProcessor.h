#pragma once
#include <JuceHeader.h>
#include <array>
class ArdenProcessor final : public juce::AudioProcessor {
public:
 ArdenProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "ARDEN";}
 bool acceptsMidi() const override {return false;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 0;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState state;
 std::atomic<float> meterIn{-80},meterOut{-80},driveIndicator{0};
 std::atomic<bool> clipping{false};
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 struct Channel { double previous=0, lowMemory=0; };
 std::array<Channel,2> channels{};
 juce::dsp::Oversampling<float> oversampler{2,2,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,false};
 juce::dsp::IIR::Filter<float> low[2],mid[2],high[2];
 juce::SmoothedValue<float> inGain,outGain,driveGain,makeupGain,modelBlend,eqBlend,powerBlend;
 double sampleRate=44100; int lastCircuit=0;
 float processCircuit(float x,int model,int ch,float drive);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArdenProcessor)
};
