#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

void ArdenEditor::TransparentKnobLook::drawRotarySlider(juce::Graphics&, int, int, int, int,
    float, float, float, juce::Slider&)
{
    // The knob face is rendered and rotated by ArdenEditor::paint().
    // This transparent slider receives mouse and automation input.
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
    power.setLookAndFeel(&toggleLook);
    eq.setLookAndFeel(&toggleLook);
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
    power.setLookAndFeel(nullptr);
    eq.setLookAndFeel(nullptr);
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
    clipHold = processor.clipping.load() ? 0.65f : juce::jmax(0.0f, clipHold - 1.0f/30.0f);
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
    // Rotate the actual photographed metal knob faces rather than drawing
    // synthetic pointers on top of static knobs. The JPEG is already embedded
    // and decoded; clipped subimages require no additional resource files.
    if (panel.isValid())
    {
        struct Face { float x, y; int radius; float defaultValue; };
        const Face faces[] = {
            {255.0f, 635.0f, 62, 0.0f},  // INPUT
            {766.0f, 635.0f, 62, 0.0f},  // DRIVE
            {229.0f, 922.0f, 48, 0.0f},  // LOW
            {513.0f, 922.0f, 48, 0.0f},  // MID
            {795.0f, 922.0f, 48, 0.0f},  // HIGH
            {512.0f, 1366.0f, 63, 0.0f} // OUTPUT
        };
        for (int i = 0; i < 6; ++i)
        {
            const auto& f = faces[i];
            const int r = f.radius;
            const int x = juce::roundToInt(f.x) - r;
            const int y = juce::roundToInt(f.y) - r;
            const auto knobFace = panel.getClippedImage({x, y, 2*r, 2*r});
            const auto& slider = knobs[(size_t)i];
            const auto range = slider.getMaximum() - slider.getMinimum();
            if (range <= 0.0) continue;
            const float normal = (float)((slider.getValue() - slider.getMinimum()) / range);
            const float normalDefault = (float)((f.defaultValue - slider.getMinimum()) / range);
            const float rotation = (normal - normalDefault) * juce::MathConstants<float>::pi * 1.5f;
            juce::Graphics::ScopedSaveState knobSave(g);
            juce::Path circle;
            circle.addEllipse(f.x-r+1.0f, f.y-r+1.0f, 2.0f*r-2.0f, 2.0f*r-2.0f);
            g.reduceClipRegion(circle);
            g.drawImageTransformed(knobFace,
                juce::AffineTransform::rotation(rotation, (float)r, (float)r)
                    .translated((float)x, (float)y), false);
        }

        // The six-position frequency selector rotates in discrete steps.
        const auto* selectedFreq = processor.state.getRawParameterValue("midfreq");
        const int selected = selectedFreq ? juce::jlimit(0, 5, (int)std::round(selectedFreq->load())) : 2;
        constexpr int radius = 44;
        constexpr int fx = 512, fy = 1165;
        const auto selectorFace = panel.getClippedImage({fx-radius, fy-radius, radius*2, radius*2});
        {
        juce::Graphics::ScopedSaveState selectorSave(g);
        juce::Path selectorCircle;
        selectorCircle.addEllipse((float)(fx-radius+1), (float)(fy-radius+1),
                                  (float)(2*radius-2), (float)(2*radius-2));
        g.reduceClipRegion(selectorCircle);
        const float angle = (selected - 2) * (juce::MathConstants<float>::pi * 1.5f / 5.0f);
        g.drawImageTransformed(selectorFace,
            juce::AffineTransform::rotation(angle, (float)radius, (float)radius)
                .translated((float)(fx-radius), (float)(fy-radius)), false);
        }
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
    // Dynamic lamps are painted over the photographed lamps. They read actual
    // DSP metering and APVTS parameters, not decorative animation.
    const auto* eqParameter = processor.state.getRawParameterValue("eq");
    const auto* powerParameter = processor.state.getRawParameterValue("power");
    const auto* meterParameter = processor.state.getRawParameterValue("meter");
    const bool eqEnabled = eqParameter && eqParameter->load() > 0.5f;
    const bool powerEnabled = powerParameter && powerParameter->load() > 0.5f;
    const bool meterOut = meterParameter && meterParameter->load() > 0.5f;

    auto lamp = [&](float cx, float cy, float radius, juce::Colour on,
                    float brightness)
    {
        brightness = juce::jlimit(0.0f, 1.0f, brightness);
        // Opaque dark socket masks the static illuminated JPEG lamp.
        g.setColour(juce::Colour(0xff211b15));
        g.fillEllipse(cx-radius-3.0f, cy-radius-3.0f,
                      2.0f*(radius+3.0f), 2.0f*(radius+3.0f));
        g.setColour(juce::Colour(0xff6b5640));
        g.drawEllipse(cx-radius-2.0f, cy-radius-2.0f,
                      2.0f*(radius+2.0f), 2.0f*(radius+2.0f), 2.0f);
        if (brightness > 0.015f)
        {
            const auto glow = on.withAlpha(0.34f * brightness);
            g.setColour(glow);
            g.fillEllipse(cx-radius*1.7f, cy-radius*1.7f,
                          radius*3.4f, radius*3.4f);
        }
        juce::ColourGradient lens(
            juce::Colour(0xff42372a).interpolatedWith(on.brighter(0.65f), brightness),
            cx-radius*0.3f, cy-radius*0.5f,
            juce::Colour(0xff1a1614).interpolatedWith(on.darker(0.55f), brightness),
            cx+radius, cy+radius, true);
        g.setGradientFill(lens);
        g.fillEllipse(cx-radius, cy-radius, radius*2.0f, radius*2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.10f + brightness*0.48f));
        g.fillEllipse(cx-radius*0.55f, cy-radius*0.68f,
                      radius*0.62f, radius*0.46f);
    };

    // These positions correspond to the SAT and CLIP bulbs in the JPEG.
    // At silence SAT is dark; the processor reports its saturation amount.
    const float saturation = powerEnabled
        ? juce::jlimit(0.0f, 1.0f, processor.driveIndicator.load() * 2.5f)
        : 0.0f;
    lamp(794.0f, 162.0f, 13.0f, juce::Colour(0xffffad18), saturation);
    lamp(862.0f, 162.0f, 13.0f, juce::Colour(0xffed261b),
         clipHold > 0.0f ? 1.0f : 0.0f);
    // POWER is a real state indicator, not a permanently lit JPEG button.
    lamp(812.0f, 1380.0f, 31.0f, juce::Colour(0xffffa722),
         powerEnabled ? 1.0f : 0.0f);

    // Distinct switch state markers are kept outside the hit targets.
    auto status = [&](float x, float y, bool enabled)
    {
        g.setColour(enabled ? juce::Colour(0xfff6bb54)
                            : juce::Colour(0xff3b3530));
        g.fillEllipse(x, y, 13.0f, 13.0f);
        g.setColour(juce::Colour(0xff221b16));
        g.drawEllipse(x, y, 13.0f, 13.0f, 1.5f);
    };
    status(210.0f, 1467.0f, eqEnabled);
    status(902.0f, meterOut ? 426.0f : 325.0f, true);
    if (!powerEnabled)
    {
        // Subtle power-off dimming, without obscuring controls completely.
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillRect(35, 22, 954, 1485);
    }

}
juce::AudioProcessorEditor* ArdenProcessor::createEditor() { return new ArdenEditor(*this); }
