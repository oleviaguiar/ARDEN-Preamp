#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {float db(float x){return juce::Decibels::decibelsToGain(x);} float val(const juce::AudioProcessorValueTreeState& s,const char* id){return s.getRawParameterValue(id)->load();}}
juce::AudioProcessorValueTreeState::ParameterLayout ArdenProcessor::layout(){
 using namespace juce; std::vector<std::unique_ptr<RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float a,float b,float initial,float step=0.01f){p.push_back(std::make_unique<AudioParameterFloat>(ParameterID{id,1},name,NormalisableRange<float>(a,b,step),initial));};
 f("input","Input",-24,24,0);f("drive","Drive",0,10,0);f("output","Output",-24,12,0);
 f("low","Low",-6,6,0);f("mid","Mid",-6,6,0);f("high","High",-6,6,0);
 p.push_back(std::make_unique<AudioParameterChoice>(ParameterID{"midfreq",1},"Mid Frequency",StringArray{"360 Hz","700 Hz","1.6 kHz","3.2 kHz","4.8 kHz","7.2 kHz"},2));
 p.push_back(std::make_unique<AudioParameterChoice>(ParameterID{"circuit",1},"Circuit",StringArray{"1073","Valve"},0));
 p.push_back(std::make_unique<AudioParameterChoice>(ParameterID{"meter",1},"Meter",StringArray{"IN","OUT"},1));
 p.push_back(std::make_unique<AudioParameterBool>(ParameterID{"eq",1},"EQ Enabled",true));
 p.push_back(std::make_unique<AudioParameterBool>(ParameterID{"power",1},"Power",true));
 return {p.begin(),p.end()};
}
ArdenProcessor::ArdenProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"ARDEN",layout()){}
void ArdenProcessor::prepareToPlay(double sr,int block){
 sampleRate=sr; oversampler.reset();oversampler.initProcessing(static_cast<size_t>(juce::jmax(32,block)));
 setLatencySamples(static_cast<int>(std::ceil(oversampler.getLatencyInSamples())));
 for(auto& c:channels)c={};
 for(auto* s:{&inGain,&outGain,&driveGain,&makeupGain,&modelBlend,&eqBlend,&powerBlend})s->reset(sr,0.025);
 inGain.setCurrentAndTargetValue(1);outGain.setCurrentAndTargetValue(1);driveGain.setCurrentAndTargetValue(0);makeupGain.setCurrentAndTargetValue(1);
 modelBlend.setCurrentAndTargetValue(val(state,"circuit"));eqBlend.setCurrentAndTargetValue(val(state,"eq"));powerBlend.setCurrentAndTargetValue(val(state,"power"));
 for(int c=0;c<2;c++){low[c].reset();mid[c].reset();high[c].reset();}
}
float ArdenProcessor::processCircuit(float x,int model,int ch,float drive){
 auto& c=channels[(size_t)ch];
 // Two deliberately different, low-cost nonlinear approximations; not measured hardware models.
 const float mismatch=ch==0?1.0f:1.007f;
 if(model==0){
  // Class-A-like asymmetric harmonics plus transformer-inspired low-frequency hysteresis approximation.
  float t=x*(1.0f+drive*0.42f)*mismatch;
  float h=std::tanh(t+0.13f)-std::tanh(0.13f);
  float curvature=0.065f*(t*t/(1.0f+t*t))*((t>=0)?1.0f:-0.6f);
  c.lowMemory+=0.003*(h-c.lowMemory);
  return (h+curvature+0.045f*c.lowMemory)/(1.0f+drive*0.09f);
 }
 float t=x*(1.0f+drive*0.55f)*mismatch;
 float positive=std::tanh(t*0.88f+0.24f)-std::tanh(0.24f);
 float negative=std::tanh(t*1.12f-0.14f)+std::tanh(0.14f);
 return (0.60f*positive+0.40f*negative)/(1.0f+drive*0.10f);
}
void ArdenProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals noDenormals;
 int n=buffer.getNumSamples(),count=juce::jmin(2,buffer.getNumChannels());if(n==0||count==0)return;
 for(int ch=count;ch<buffer.getNumChannels();++ch)buffer.clear(ch,0,n);
 float input=val(state,"input"),drive=val(state,"drive"),output=val(state,"output");
 inGain.setTargetValue(db(input));outGain.setTargetValue(db(output));driveGain.setTargetValue(drive);
 // Static approximate level matching; no lookahead or compression pumping.
 makeupGain.setTargetValue(db(-2.0f*std::log1p(drive)));
 modelBlend.setTargetValue(val(state,"circuit"));eqBlend.setTargetValue(val(state,"eq"));powerBlend.setTargetValue(val(state,"power"));
 constexpr float frequencies[]={360,700,1600,3200,4800,7200};
 int freq=juce::jlimit(0,5,(int)val(state,"midfreq"));
 auto l=juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate,110.0f,0.72f,db(val(state,"low")));
 auto m=juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,frequencies[freq],0.65f,db(val(state,"mid")));
 auto h=juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate,12000.0f,0.68f,db(val(state,"high")));
 for(int c=0;c<count;c++){*low[c].coefficients=*l;*mid[c].coefficients=*m;*high[c].coefficients=*h;}
 float inPeak=0,outPeak=0,energyIn=0,energyOut=0,sat=0;
 // Work on 4x oversampled signal; maintain original buffer for power bypass.
 juce::AudioBuffer<float> dry(count,n);for(int c=0;c<count;c++)dry.copyFrom(c,0,buffer,c,0,n);
 for(int c=0;c<count;c++)for(int i=0;i<n;i++){float x=buffer.getSample(c,i);energyIn+=x*x;inPeak=juce::jmax(inPeak,std::abs(x));}
 auto up=oversampler.processSamplesUp(juce::dsp::AudioBlock<float>(buffer).getSubBlock(0,(size_t)n));
 int upN=(int)up.getNumSamples();
 // Sample-rate conversion is handled by JUCE; nonlinear processing runs at 4x.
 for(int i=0;i<upN;i++){
  float ig=inGain.getNextValue(),d=driveGain.getNextValue(),mg=makeupGain.getNextValue(),mix=modelBlend.getNextValue();
  for(int c=0;c<count;c++){
   float x=up.getChannelPointer(c)[i]*ig;
   float a=processCircuit(x,0,c,d);
   float b=processCircuit(x,1,c,d);
   float y=juce::jmap(mix,a,b)*mg;
   sat+=std::abs(y-x);
   up.getChannelPointer(c)[i]=y;
  }
 }
 oversampler.processSamplesDown(juce::dsp::AudioBlock<float>(buffer).getSubBlock(0,(size_t)n));
 for(int i=0;i<n;i++){
  float e=eqBlend.getNextValue(),o=outGain.getNextValue(),p=powerBlend.getNextValue();
  for(int c=0;c<count;c++){
   float x=buffer.getSample(c,i),filtered=high[c].processSample(mid[c].processSample(low[c].processSample(x)));
   float wet=(x+(filtered-x)*e)*o;
   float y=dry.getSample(c,i)+(wet-dry.getSample(c,i))*p;
   buffer.setSample(c,i,y);energyOut+=y*y;outPeak=juce::jmax(outPeak,std::abs(y));
  }
 }
 float scale=1.0f/(float)(n*count);
 meterIn.store(juce::Decibels::gainToDecibels(std::sqrt(energyIn*scale),-80.0f));
 meterOut.store(juce::Decibels::gainToDecibels(std::sqrt(energyOut*scale),-80.0f));
 driveIndicator.store(juce::jlimit(0.0f,1.0f,sat/(float)(upN*count)*2.0f));
 clipping.store(outPeak>=1.0f);
}
void ArdenProcessor::getStateInformation(juce::MemoryBlock& out){auto xml=state.copyState().createXml();copyXmlToBinary(*xml,out);}
void ArdenProcessor::setStateInformation(const void* data,int size){auto xml=getXmlFromBinary(data,size);if(xml&&xml->hasTagName(state.state.getType()))state.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ArdenProcessor();}
