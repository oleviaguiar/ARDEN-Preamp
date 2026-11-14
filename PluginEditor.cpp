#include "PluginEditor.h"
#include <cmath>
ArdenEditor::ArdenEditor(ArdenProcessor& p):AudioProcessorEditor(&p),processor(p){
 constexpr const char* ids[]={"input","drive","low","mid","high","output","midfreq"};
 for(int i=0;i<6;i++){
  auto& s=knobs[(size_t)i];s.setSliderStyle(juce::Slider::RotaryVerticalDrag);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
  s.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
  addAndMakeVisible(s);knobLinks[(size_t)i]=std::make_unique<SliderAttachment>(p.state,ids[i],s);
 }
 for(auto* c:{&circuit,&freq,&meter}){addAndMakeVisible(*c);c->setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff344957));c->setColour(juce::ComboBox::textColourId,juce::Colours::ivory);}
 circuit.addItem("1073",1);circuit.addItem("VALVE",2);
 const char* fs[]={"360 Hz","700 Hz","1.6 kHz","3.2 kHz","4.8 kHz","7.2 kHz"};for(int i=0;i<6;i++)freq.addItem(fs[i],i+1);
 meter.addItem("IN",1);meter.addItem("OUT",2);
 circuitLink=std::make_unique<ComboAttachment>(p.state,"circuit",circuit);
 freqLink=std::make_unique<ComboAttachment>(p.state,"midfreq",freq);
 meterLink=std::make_unique<ComboAttachment>(p.state,"meter",meter);
 for(auto* b:{&power,&eq}){addAndMakeVisible(*b);b->setColour(juce::ToggleButton::textColourId,juce::Colours::ivory);}
 powerLink=std::make_unique<ButtonAttachment>(p.state,"power",power);eqLink=std::make_unique<ButtonAttachment>(p.state,"eq",eq);
 setResizable(true,true);setResizeLimits(300,650,760,1640);setSize(380,820);startTimerHz(30);
}
void ArdenEditor::resized(){
 auto b=getLocalBounds().toFloat();float s=juce::jmin(b.getWidth()/380.0f,b.getHeight()/820.0f);
 auto place=[&](juce::Component& c,float x,float y,float w,float h){c.setBounds(juce::Rectangle<int>(juce::roundToInt((b.getWidth()-380*s)/2+x*s),juce::roundToInt((b.getHeight()-820*s)/2+y*s),juce::roundToInt(w*s),juce::roundToInt(h*s)));};
 place(meter,235,207,78,27);place(circuit,107,254,166,31);
 place(knobs[0],145,301,90,82);place(knobs[1],145,394,90,82);
 place(knobs[2],145,501,90,67);place(knobs[3],145,575,90,67);place(freq,236,597,112,25);
 place(knobs[4],145,650,90,67);place(knobs[5],145,724,90,67);
 place(power,21,788,95,25);place(eq,266,788,95,25);
}
void ArdenEditor::timerCallback(){float target=(processor.state.getRawParameterValue("meter")->load()<0.5f?processor.meterIn.load():processor.meterOut.load());displayedVU+=0.22f*(target-displayedVU);if(processor.clipping.load())clipHold=2.0f;else clipHold=juce::jmax(0.0f,clipHold-1.0f/30.0f);repaint();}
void ArdenEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff131a20));
 auto b=getLocalBounds().toFloat();float s=juce::jmin(b.getWidth()/380.0f,b.getHeight()/820.0f);
 juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::translation((b.getWidth()-380*s)*0.5f,(b.getHeight()-820*s)*0.5f).scaled(s));
 juce::ColourGradient metal(juce::Colour(0xff526d7b),0,0,juce::Colour(0xff293e4b),380,820,false);metal.addColour(0.48,juce::Colour(0xff425c69));g.setGradientFill(metal);g.fillRoundedRectangle(5,5,370,810,10);
 g.setColour(juce::Colour(0xffa0a9a5).withAlpha(0.4f));g.drawRoundedRectangle(7,7,366,806,9,2);
 for(int i=0;i<4;i++){float x=(i%2)?360.f:20.f,y=(i/2)?798.f:22.f;g.setColour(juce::Colour(0xff182c37));g.fillEllipse(x-5,y-5,10,10);g.setColour(juce::Colour(0xffa6a9a2));g.drawLine(x-3,y+2,x+3,y-2,1.2f);}
 g.setColour(juce::Colour(0xfff4eee0));g.setFont(juce::Font(juce::FontOptions(37).withStyle("Bold")));g.drawText("ARDEN",0,30,380,46,juce::Justification::centred);
 g.setFont(juce::Font(juce::FontOptions(12)));g.drawText("DUAL CIRCUIT PREAMPLIFIER",0,77,380,22,juce::Justification::centred);
 g.setColour(juce::Colour(0xff142a33));g.fillRoundedRectangle(45,111,290,129,7);
 juce::ColourGradient glass(juce::Colour(0xffffe4a4),52,116,juce::Colour(0xffb88d51),320,234,false);g.setGradientFill(glass);g.fillRoundedRectangle(54,119,272,111,5);
 g.setColour(juce::Colour(0xff453c2b));g.setFont(juce::Font(juce::FontOptions(16).withStyle("Bold")));g.drawText("VU",155,136,70,25,juce::Justification::centred);
 for(int i=0;i<=10;i++){float a=juce::jmap((float)i,0.0f,10.0f,-2.45f,-0.69f);float x=190+88*std::cos(a),y=254+88*std::sin(a);g.drawLine(x,y,x+9*std::cos(a),y+9*std::sin(a),1);}
 float vu=juce::jlimit(-40.0f,5.0f,displayedVU+18.0f);float angle=juce::jmap(vu,-40.0f,5.0f,-2.45f,-0.69f);
 g.setColour(juce::Colour(0xff5b2721));g.drawLine(190,254,190+98*std::cos(angle),254+98*std::sin(angle),2);
 g.setColour(juce::Colours::white.withAlpha(0.13f));g.drawLine(62,123,295,123,3);
 g.setColour(clipHold>0?juce::Colours::red:juce::Colour(0xff491d1d));g.fillEllipse(307,98,10,10);
 float saturation=processor.driveIndicator.load();g.setColour(juce::Colour(0xff4d3210).interpolatedWith(juce::Colour(0xffffb635),saturation));g.fillEllipse(287,98,10,10);
 g.setColour(juce::Colour(0xfff0e5cf));g.setFont(juce::Font(juce::FontOptions(12).withStyle("Bold")));
 const char* labels[]={"INPUT","DRIVE GAIN","LOW","MID","HIGH","OUTPUT"};const int ys[]={289,382,487,561,636,710};
 for(int i=0;i<6;i++){g.drawText(labels[i],20,ys[i],340,18,juce::Justification::centred);}
 g.setColour(juce::Colour(0xffd7d0bb));g.drawLine(48,483,332,483,1);g.setFont(juce::Font(juce::FontOptions(11)));g.drawText("EQUALIZER",20,472,340,14,juce::Justification::centred);
 g.setFont(juce::Font(juce::FontOptions(10)));g.drawText("MID FREQ",245,578,100,18,juce::Justification::centred);
 g.drawText("SAT",282,87,23,12,juce::Justification::centred);g.drawText("CLIP",303,87,30,12,juce::Justification::centred);
}
juce::AudioProcessorEditor* ArdenProcessor::createEditor(){return new ArdenEditor(*this);}
