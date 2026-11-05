#include "PluginEditor.h"
using namespace juce;
using namespace baak;
static juce::Colour bg(0xff10171b), panel(0xff192328), line(0xff2e3c42), text(0xffeef4f2), muted(0xff9aafb5),
    accent(0xff85e0c0);
BaakEditor::Theme::Theme() {
    setColour(Slider::textBoxTextColourId, text);
    setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour(TextButton::buttonColourId, line);
    setColour(TextButton::textColourOffId, text);
    setColour(ComboBox::backgroundColourId, panel);
    setColour(ComboBox::textColourId, text);
    setColour(PopupMenu::backgroundColourId, panel);
    setColour(PopupMenu::textColourId, text);
}
void BaakEditor::Theme::drawRotarySlider(Graphics &g, int x, int y, int w, int h, float p, float a, float b,
                                         Slider &) {
    auto bounds = Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                                   static_cast<float>(h))
                      .reduced(9);
    float r = jmin(bounds.getWidth(), bounds.getHeight()) * .5f;
    auto c = bounds.getCentre();
    Path track, fill;
    track.addCentredArc(c.x, c.y, r, r, 0, a, b, true);
    fill.addCentredArc(c.x, c.y, r, r, 0, a, a + p * (b - a), true);
    g.setColour(line);
    g.strokePath(track, PathStrokeType(4));
    g.setColour(accent);
    g.strokePath(fill, PathStrokeType(4));
    g.setColour(bg.brighter(.04f));
    g.fillEllipse(c.x - r + 7, c.y - r + 7, 2 * r - 14, 2 * r - 14);
    float angle = a + p * (b - a) - MathConstants<float>::halfPi;
    g.setColour(text);
    g.drawLine(c.x + std::cos(angle) * (r - 17), c.y + std::sin(angle) * (r - 17),
               c.x + std::cos(angle) * (r - 10), c.y + std::sin(angle) * (r - 10), 2);
}
BaakEditor::Knob::Knob(BaakProcessor &p, int i) {
    auto &s = specs[i];
    addAndMakeVisible(slider);
    addAndMakeVisible(title);
    title.setText(s.name, dontSendNotification);
    title.setJustificationType(Justification::centred);
    title.setColour(Label::textColourId, text);
    title.setFont(Font(FontOptions(13.f)));
    slider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(Slider::TextBoxBelow, false, 105, 23);
    slider.setTextValueSuffix(s.unit);
    slider.setDoubleClickReturnValue(true, s.initial);
    slider.setTooltip(s.help);
    slider.setTitle(s.name);
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state, s.id, slider);
}
void BaakEditor::Knob::resized() {
    auto r = getLocalBounds();
    title.setBounds(r.removeFromTop(22));
    slider.setBounds(r);
}
BaakEditor::BaakEditor(BaakProcessor &p) : AudioProcessorEditor(p), processor(p) {
    setLookAndFeel(&theme);
    history.fill(-60);
    for (auto *b : {&mode, &bypass, &apply, &undo, &freeze, &reset})
        addAndMakeVisible(*b);
    mode.onClick = [this] {
        pro = !pro;
        mode.setButtonText(pro ? "Simple controls" : "Pro controls");
        resized();
        repaint();
    };
    bypass.onClick = [this] { processor.setValue(Bypass, processor.current()[Bypass] > .5f ? 0.f : 1.f); };
    reset.onClick = [this] {
        processor.resetMeters.store(true);
        history.fill(-60);
    };
    freeze.onClick = [this] {
        paused = !paused;
        freeze.setButtonText(paused ? "Resume advice" : "Pause advice");
    };
    apply.onClick = [this] {
        if (suggested < 0)
            return;
        previous = processor.current();
        hasUndo = true;
        processor.setValue(suggested, suggestionValue);
    };
    undo.onClick = [this] {
        if (!hasUndo)
            return;
        for (int i = 0; i < Count; ++i)
            processor.setValue(i, previous[i]);
        hasUndo = false;
    };
    addAndMakeVisible(presets);
    presets.addItem("Start clean", 1);
    presets.addItem("Gentle polish", 2);
    presets.addItem("Warm & intimate", 3);
    presets.addItem("Punch & space", 4);
    presets.setSelectedId(1, dontSendNotification);
    presets.onChange = [this] {
        auto p = defaults();
        int id = presets.getSelectedId();
        if (id == 2) {
            p[High] = .7f;
            p[Glue] = 15;
            p[Calm] = 10;
        }
        if (id == 3) {
            p[Low] = .6f;
            p[Warmth] = 18;
            p[Glue] = 20;
            p[High] = -.5f;
        }
        if (id == 4) {
            p[Width] = 110;
            p[LowComp] = 12;
            p[Glue] = 10;
        }
        previous = processor.current();
        hasUndo = true;
        for (int i = 0; i < Count; ++i)
            processor.setValue(i, p[i]);
    };
    for (int i = 0; i < 3; ++i) {
        auto &s = slots[i];
        addAndMakeVisible(s);
        s.setButtonText(String::charToString(static_cast<juce_wchar>('A' + i)));
        s.setTooltip("Click to recall; Shift-click to store current settings.");
        s.onClick = [this, i] {
            if (ModifierKeys::getCurrentModifiers().isShiftDown())
                processor.snapshot(i);
            else
                processor.recall(i);
        };
    }
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&controls, false);
    viewport.setScrollBarsShown(true, false);
    for (int i = 0; i < Count - 1; ++i) {
        auto k = std::make_unique<Knob>(processor, i);
        controls.addAndMakeVisible(*k);
        knobs.push_back(std::move(k));
    }
    addAndMakeVisible(advice);
    addAndMakeVisible(explanation);
    advice.setColour(Label::textColourId, text);
    advice.setFont(Font(FontOptions(19.f)));
    advice.setJustificationType(Justification::topLeft);
    explanation.setColour(Label::textColourId, muted);
    explanation.setFont(Font(FontOptions(14.f)));
    explanation.setJustificationType(Justification::topLeft);
    setSize(1100, 750);
    setResizable(true, true);
    setResizeLimits(1000, 720, 1450, 1000);
    startTimerHz(10);
    updateAdvice();
}
BaakEditor::~BaakEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}
void BaakEditor::resized() {
    int w = getWidth(), h = getHeight(), right = w - 310;
    presets.setBounds(w - 510, 25, 180, 32);
    mode.setBounds(w - 315, 25, 140, 32);
    bypass.setBounds(w - 160, 25, 130, 32);
    viewport.setBounds(24, 350, right - 48, h - 410);
    int cw = viewport.getWidth() - 16, columns = 5, n = pro ? Count - 1 : 10;
    controls.setSize(cw, ((n + columns - 1) / columns) * 138);
    for (int i = 0; i < Count - 1; ++i) {
        knobs[i]->setVisible(i < n);
        knobs[i]->setBounds((i % columns) * cw / columns, (i / columns) * 138, cw / columns, 130);
    }
    advice.setBounds(right + 22, 400, 260, 80);
    explanation.setBounds(right + 22, 478, 260, 118);
    apply.setBounds(right + 22, h - 140, 260, 34);
    undo.setBounds(right + 22, h - 100, 125, 30);
    freeze.setBounds(right + 157, h - 100, 125, 30);
    for (int i = 0; i < 3; ++i)
        slots[i].setBounds(24 + i * 40, h - 46, 34, 25);
    reset.setBounds(right - 145, h - 46, 120, 25);
}
void BaakEditor::paint(Graphics &g) {
    int w = getWidth(), h = getHeight(), right = w - 310;
    g.fillAll(bg);
    g.setColour(accent);
    g.setFont(Font(FontOptions(12.f, Font::bold)));
    g.drawText("BAAKPRODUCTION", 24, 20, 260, 20, Justification::left);
    g.setColour(text);
    g.setFont(Font(FontOptions(27.f, Font::bold)));
    g.drawText("Master", 24, 39, 230, 35, Justification::left);
    g.setColour(muted);
    g.setFont(Font(FontOptions(12.f)));
    g.drawText("MIX-BUS & MASTERING PROCESSOR   /   ALPHA 0.1", 24, 82, 650, 20, Justification::left);
    g.setColour(panel);
    g.fillRoundedRectangle(24, 116, static_cast<float>(w - 48), 208, 12);
    auto metric = [&](int x, String label, float value, String unit) {
        g.setColour(muted);
        g.setFont(Font(FontOptions(12.f)));
        g.drawText(label, x, 135, 175, 20, Justification::left);
        g.setColour(text);
        g.setFont(Font(FontOptions(30.f)));
        g.drawText(value < -80 ? String("--") : String(value, 1), x, 161, 140, 38, Justification::left);
        g.setFont(Font(FontOptions(11.f)));
        g.setColour(muted);
        g.drawText(unit, x, 201, 175, 20, Justification::left);
    };
    metric(46, "LOUDNESS", processor.outShort.load(), "LUFS-S / 3 SECOND WINDOW");
    metric(225, "PEAK", processor.outPeak.load(), "dBFS / SAMPLE PEAK");
    metric(405, "LIMITING", processor.gainReduction.load(), "dB / GAIN REDUCTION");
    g.setColour(muted);
    g.setFont(Font(FontOptions(12.f)));
    g.drawText("OUTPUT LOUDNESS HISTORY", 610, 135, w - 660, 20, Justification::left);
    Rectangle<float> chart(610, 172, static_cast<float>(w - 662), 110);
    g.setColour(line);
    for (int i = 0; i < 4; ++i)
        g.drawHorizontalLine(static_cast<int>(chart.getY() + i * chart.getHeight() / 3), chart.getX(),
                             chart.getRight());
    Path path;
    for (int i = 0; i < 220; ++i) {
        float v = history[(historyIndex + i) % 220];
        float x = chart.getX() + chart.getWidth() * i / 219,
              y = chart.getBottom() - jlimit(0.f, 1.f, (v + 48) / 48) * chart.getHeight();
        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }
    g.setColour(accent);
    g.strokePath(path, PathStrokeType(2));
    g.setColour(muted);
    g.setFont(Font(FontOptions(12.f)));
    g.drawText("Integrated: " + String(processor.outIntegrated.load(), 1) +
                   " LUFS    |    Stereo correlation: " + String(processor.correlation.load(), 2) +
                   "    |    2x oversampling",
               46, 269, 550, 30, Justification::left);
    g.setColour(text);
    g.setFont(Font(FontOptions(14.f, Font::bold)));
    g.drawText(pro ? "SHAPE YOUR SOUND  /  PRO" : "SHAPE YOUR SOUND  /  SIMPLE", 24, 327, right - 40, 24,
               Justification::left);
    g.setColour(panel);
    g.fillRoundedRectangle(static_cast<float>(right), 350, 286, static_cast<float>(h - 410), 12);
    g.setColour(accent);
    g.setFont(Font(FontOptions(12.f, Font::bold)));
    g.drawText("MASTER COACH", right + 22, 369, 250, 20, Justification::left);
    g.setColour(muted);
    g.setFont(Font(FontOptions(11.f)));
    g.drawText("Shift-click A / B / C to save", 155, h - 45, 230, 22, Justification::left);
    g.drawText("Local rules / No trained AI", right + 8, h - 48, 290, 28,
               Justification::centred);
}
void BaakEditor::timerCallback() {
    history[historyIndex] = processor.outShort.load();
    historyIndex = (historyIndex + 1) % 220;
    bypass.setButtonText(processor.current()[Bypass] > .5f ? "Bypassed" : "Bypass");
    if (!paused)
        updateAdvice();
    undo.setEnabled(hasUndo);
    repaint();
}
void BaakEditor::updateAdvice() {
    auto p = processor.current();
    suggested = -1;
    String title, detail;
    if (processor.elapsed.load() < 6 || processor.outShort.load() < -65) {
        title = "Play your track";
        detail = "Let a representative section play for at least 6 seconds. Advice is based on measured "
                 "audio, not genre guesses.";
    } else if (p[Bypass] > .5f) {
        title = "Processing is bypassed";
        detail = "Enable processing to evaluate your master. Bypass is aligned to the plug-in's latency.";
    } else if (processor.gainReduction.load() > 4 && p[Loud] > 1) {
        title = "Give the transients room";
        detail = "The limiter is reducing more than 4 dB in this block. Try 1 dB less Loudness, then compare "
                 "the kick and snare.";
        suggested = Loud;
        suggestionValue = std::max(0.f, p[Loud] - 1);
    } else if (processor.correlation.load() < 0.1f && p[Width] > 100) {
        title = "Check mono compatibility";
        detail = "Low correlation can mean fragile stereo information. Try returning Width to 100%, then "
                 "check a mono fold-down.";
        suggested = Width;
        suggestionValue = 100;
    } else if (p[Ceiling] > -1) {
        title = "Leave some peak margin";
        detail = "Try a -1 dBFS sample ceiling. This is not a true-peak guarantee; verify the final export "
                 "with a true-peak meter.";
        suggested = Ceiling;
        suggestionValue = -1;
    } else if (processor.outShort.load() < -18 && p[Loud] < 12) {
        title = "More level is available";
        detail = "This section is relatively quiet. If more loudness suits your music, audition +1 dB. There "
                 "is no universal LUFS target.";
        suggested = Loud;
        suggestionValue = p[Loud] + 1;
    } else {
        title = "Listen to the balance";
        detail = "No obvious level or width issue in the current measurements. Compare bass, vocal clarity "
                 "and punch across the whole song. A meter cannot certify a finished master.";
    }
    advice.setText(title, dontSendNotification);
    explanation.setText(detail, dontSendNotification);
    apply.setEnabled(suggested >= 0);
}
