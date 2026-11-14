#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

void ArdenEditor::TransparentKnobLook::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
    float position, float startAngle, float endAngle, juce::Slider&)
{
    // Transparent control: the photographed metal knob remains visible underneath.
    // A moving dark indicator and tiny reflection communicate the actual value.
    auto cx = x + width * 0.5f, cy = y + height * 0.5f;
    auto r = juce::jmin(width, height) * 0.34f;
    auto angle = startAngle + position * (endAngle - startAngle);
    juce::Point<float> tip(cx + std::sin(angle) * r, cy - std::cos(angle) * r);
    juce::Point<float> base(cx + std::sin(angle) * r * 0.49f, cy - std::cos(angle) * r * 0.49f);
    g.setColour(juce::Colour(0xff171716).withAlpha(0.9f));
    g.drawLine(base.x, base.y, tip.x, tip.y, juce::jmax(2.0f, width * 0.032f));
    g.setColour(juce::Colour(0xfff2d9aa).withAlpha(0.72f));
    g.drawLine(base.x + 1.2f, base.y, tip.x + 1.2f, tip.y, 0.8f);
}

ArdenEditor::ArdenEditor(ArdenProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    // Decode the JPEG resource embedded by juce_add_binary_data.
    // JPEG avoids the PNG decoder path that failed on the previous build.
    // The resource is compiled into the VST3; no external image is needed.
    juce::JPEGImageFormat jpeg;
    juce::MemoryInputStream imageStream(BinaryData::ArdenPanel_jpg,
                                        static_cast<size_t>(BinaryData::ArdenPanel_jpgSize), false);
    panel = jpeg.decodeImage(imageStream);
    // If decoding fails, the existing fallback panel remains visible.
    const char* ids[] = {"input", "drive", "low", "mid", "high", "output"};
    for (int i = 0; i < 6; ++i)
    {
        auto& k = knobs[(size_t)i];
        k.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        k.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                              juce::MathConstants<float>::pi * 2.75f, true);
        k.setLookAndFeel(&knobLook);
        k.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(k);
        knobLinks[(size_t)i] = std::make_unique<SliderAttachment>(p.state, ids[i], k);
    }
    for (auto* c : {&circuit, &freq, &meter})
    {
        addAndMakeVisible(*c);
        c->setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
        c->setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        c->setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
        c->setColour(juce::ComboBox::arrowColourId, juce::Colours::transparentBlack);
    }
    circuit.addItem("1073", 1); circuit.addItem("VALVE", 2);
    const char* fs[] = {"360 Hz", "700 Hz", "1.6 kHz", "3.2 kHz", "4.8 kHz", "7.2 kHz"};
    for (int i = 0; i < 6; ++i) freq.addItem(fs[i], i + 1);
    meter.addItem("IN", 1); meter.addItem("OUT", 2);
    circuitLink = std::make_unique<ComboAttachment>(p.state, "circuit", circuit);
    freqLink = std::make_unique<ComboAttachment>(p.state, "midfreq", freq);
    meterLink = std::make_unique<ComboAttachment>(p.state, "meter", meter);
    addAndMakeVisible(power); addAndMakeVisible(eq);
    power.setButtonText({}); eq.setButtonText({});
    power.setColour(juce::ToggleButton::tickColourId, juce::Colours::transparentBlack);
    eq.setColour(juce::ToggleButton::tickColourId, juce::Colours::transparentBlack);
    powerLink = std::make_unique<ButtonAttachment>(p.state, "power", power);
    eqLink = std::make_unique<ButtonAttachment>(p.state, "eq", eq);
    setResizable(true, true); setResizeLimits(340, 510, 1024, 1536);
    setSize(512, 768); startTimerHz(30);
}
ArdenEditor::~ArdenEditor()
{
    for (auto& k : knobs) k.setLookAndFeel(nullptr);
}
void ArdenEditor::resized()
{
    const auto b = getLocalBounds().toFloat();
    const float s = juce::jmin(b.getWidth() / 1024.0f, b.getHeight() / 1536.0f);
    const float ox = (b.getWidth() - 1024.0f * s) * 0.5f;
    const float oy = (b.getHeight() - 1536.0f * s) * 0.5f;
    auto place = [&](juce::Component& c, float x, float y, float w, float h)
    {
        c.setBounds(juce::roundToInt(ox + x*s), juce::roundToInt(oy + y*s),
                    juce::roundToInt(w*s), juce::roundToInt(h*s));
    };
    place(knobs[0], 146, 530, 218, 210); // input
    place(knobs[1], 657, 530, 218, 210); // drive
    place(knobs[2], 141, 837, 176, 175); // low
    place(knobs[3], 425, 837, 176, 175); // mid
    place(knobs[4], 707, 837, 176, 175); // high
    place(knobs[5], 415, 1271, 194, 192); // output
    place(circuit, 460, 572, 105, 139);
    place(freq, 351, 1042, 333, 151);
    place(meter, 847, 232, 83, 157);
    place(eq, 163, 1307, 94, 104);
    place(power, 759, 1290, 125, 135);
}
void ArdenEditor::timerCallback()
{
    const auto* meterParam = processor.state.getRawParameterValue("meter");
    const float target = (meterParam != nullptr && meterParam->load() < 0.5f)
        ? processor.meterIn.load() : processor.meterOut.load();
    displayedVU += 0.22f * (target - displayedVU);
    clipHold = processor.clipping.load() ? 2.0f : juce::jmax(0.0f, clipHold - 1.0f/30.0f);
    repaint();
}
void ArdenEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101719));
    const auto b = getLocalBounds().toFloat();
    const float s = juce::jmin(b.getWidth()/1024.0f, b.getHeight()/1536.0f);
    const float ox = (b.getWidth() - 1024.0f*s)*0.5f;
    const float oy = (b.getHeight() - 1536.0f*s)*0.5f;
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(s).translated(ox, oy));
    if (panel.isValid())
    {
        g.drawImage(panel, juce::Rectangle<float>(0, 0, 1024, 1536),
                    juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        // Visible hardware-style fallback, never a blank dark screen.
        juce::ColourGradient metal(juce::Colour(0xff254c55), 0, 0,
                                   juce::Colour(0xff10262e), 1024, 1536, false);
        g.setGradientFill(metal);
        g.fillRect(0, 0, 1024, 1536);
        g.setColour(juce::Colour(0xffb89a69));
        g.fillRect(0, 0, 30, 1536);
        g.fillRect(994, 0, 30, 1536);
        g.setColour(juce::Colour(0xfff4dfb9));
        g.setFont(juce::Font(juce::FontOptions(67.0f).withStyle("Bold")));
        g.drawText("ARDEN", 80, 44, 864, 110, juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(27.0f)));
        g.drawText("DUAL CIRCUIT PREAMPLIFIER", 60, 150, 904, 60,
                   juce::Justification::centred);
        g.setColour(juce::Colour(0xffe7d4a9));
        g.fillRoundedRectangle(120, 230, 784, 250, 28);
        g.setColour(juce::Colour(0xff42372b));
        g.setFont(juce::Font(juce::FontOptions(36.0f)));
        g.drawText("VU", 150, 255, 724, 65, juce::Justification::centred);
        const char* labels[] = {"INPUT", "DRIVE GAIN", "LOW", "MID", "HIGH", "OUTPUT"};
        const juce::Point<float> centres[] = {{255, 635}, {766, 635}, {229, 922},
                                              {513, 922}, {795, 922}, {512, 1366}};
        const float radii[] = {88, 88, 69, 69, 69, 88};
        for (int i = 0; i < 6; ++i)
        {
            const auto c = centres[i]; const auto r = radii[i];
            g.setColour(juce::Colour(0xffa4a6a4));
            g.fillEllipse(c.x-r, c.y-r, 2*r, 2*r);
            g.setColour(juce::Colour(0xff535b5c));
            g.drawEllipse(c.x-r, c.y-r, 2*r, 2*r, 6.0f);
            g.setColour(juce::Colour(0xfff2dfbb));
            g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
            g.drawText(labels[i], juce::roundToInt(c.x-140), juce::roundToInt(c.y-r-49),
                       280, 42, juce::Justification::centred);
        }
        g.setColour(juce::Colour(0xfff2dfbb));
        g.setFont(juce::Font(juce::FontOptions(25.0f)));
        g.drawText("1073     CIRCUIT     VALVE", 295, 733, 435, 45,
                   juce::Justification::centred);
        g.drawText("EQ IN", 115, 1440, 200, 45, juce::Justification::centred);
        g.drawText("POWER", 715, 1440, 200, 45, juce::Justification::centred);
        g.setColour(juce::Colour(0xfff3c879));
        g.setFont(juce::Font(juce::FontOptions(21.0f)));
        g.drawText("PANEL IMAGE FAILED TO LOAD", 140, 475, 744, 38,
                   juce::Justification::centred);
    }
    // Animated meter needle (photographic meter remains as its backdrop).
    const float v = juce::jlimit(-40.0f, 3.0f, displayedVU + 18.0f);
    const float a = juce::jmap(v, -40.0f, 3.0f, -0.80f, 0.72f);
    const juce::Point<float> pivot(422, 414);
    const juce::Point<float> tip(pivot.x + std::sin(a)*235.0f, pivot.y - std::cos(a)*235.0f);
    g.setColour(juce::Colour(0xff2b1c11).withAlpha(0.94f));
    g.drawLine(pivot.x, pivot.y, tip.x, tip.y, 2.5f);
    // Correct frequency values, always reflecting the saved/automated parameter.
    const char* names[] = {"360 Hz", "700 Hz", "1.6 kHz", "3.2 kHz", "4.8 kHz", "7.2 kHz"};
    const auto* freqParam = processor.state.getRawParameterValue("midfreq");
    const int fi = freqParam ? juce::jlimit(0, 5, (int)std::round(freqParam->load())) : 2;
    g.setColour(juce::Colour(0xff18272b).withAlpha(0.97f));
    g.fillRoundedRectangle(343, 1015, 338, 58, 7);
    g.setColour(juce::Colour(0xffefd7a7));
    g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
    g.drawText(juce::String("MID FREQ   ") + names[fi], 349, 1021, 326, 45, juce::Justification::centred);
    // Circuit state is shown beside the physical switch.
    const auto* circuitParam = processor.state.getRawParameterValue("circuit");
    const bool valve = circuitParam && circuitParam->load() > 0.5f;
    g.setColour(juce::Colour(0xfff4ce82));
    g.fillEllipse(554, valve ? 695.0f : 555.0f, 9, 9);
    if (clipHold > 0.0f)
    {
        g.setColour(juce::Colours::red.withAlpha(0.75f));
        g.fillEllipse(860, 101, 25, 25);
    }
    if (!power.getToggleState())
    {
        g.setColour(juce::Colours::black.withAlpha(0.48f));
        g.fillRect(35, 22, 954, 1485);
    }
}
juce::AudioProcessorEditor* ArdenProcessor::createEditor() { return new ArdenEditor(*this); }
