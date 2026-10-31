#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

ArmNongMuangLA2AAudioProcessorEditor::ArmNongMuangLA2AAudioProcessorEditor(ArmNongMuangLA2AAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p) {
    background = juce::ImageCache::getFromMemory(BinaryData::LA2A_Interface_png, BinaryData::LA2A_Interface_pngSize);
    // Start at 50% of the PNG native layout. All hit zones and controls scale in resized().
    setSize(749, 388);
    setResizable(true, true);
    setResizeLimits(375, 194, 2996, 1550);
    getConstrainer()->setFixedAspectRatio(1498.0 / 775.0);

    setupRotary(gain, "GAIN"); setupRotary(peak, "PEAK REDUCTION");
    setupRotary(attack, "ATTACK"); setupRotary(release, "RELEASE");
    addAndMakeVisible(gain); addAndMakeVisible(peak);
    // Attack/release remain automatable parameters but are not drawn on this LA-2A faceplate.
    attack.setVisible(false); release.setVisible(false);
    gain.setRange(0, 100, 0.1); peak.setRange(0, 100, 0.1);
    gain.setDoubleClickReturnValue(false, 50.0); peak.setDoubleClickReturnValue(false, 25.0); attack.setRange(0.1, 10, 0.01); release.setRange(50, 5000, 1);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "gain", gain);
    peakAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "peak", peak);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "attack", attack);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, "release", release);

    addHitButton(topPower, "top power"); topPower.setTooltip("Power: ON / OFF"); topPower.onClick = [this] { setPower(!powerOn); };
    addHitButton(preset, "preset menu"); preset.onClick = [this] {
        juce::PopupMenu menu; menu.addItem(1, "Default (Gain 50 / Peak Reduction 25)"); menu.addItem(2, "Save current settings to A"); menu.addItem(3, "Save current settings to B");
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int id) { if (id == 1) { processor.parameters.getParameter("gain")->setValueNotifyingHost(0.50f); processor.parameters.getParameter("peak")->setValueNotifyingHost(0.25f); } else if (id == 2) { saveSnapshot(0); status.setText("Current settings saved to A", juce::dontSendNotification); } else if (id == 3) { saveSnapshot(1); status.setText("Current settings saved to B", juce::dontSendNotification); } });
    };
    addHitButton(modelPicker, "load NAM model"); modelPicker.onClick = [this] { loadModelDialog(); };
    addHitButton(buttonA, "snapshot A"); buttonA.onClick = [this] { restoreSnapshot(0); };
    addHitButton(buttonB, "snapshot B"); buttonB.onClick = [this] { restoreSnapshot(1); };
    addHitButton(copyAB, "copy A to B"); copyAB.onClick = [this] { saveSnapshot(0); saveSnapshot(1); status.setText("Snapshot A copied to B", juce::dontSendNotification); };
    addHitButton(settings, "settings"); settings.onClick = [this] {
        juce::PopupMenu menu; menu.addItem(1, "Reset controls"); menu.addItem(2, "Unload NAM model");
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int id) {
            if (id == 1) { processor.parameters.getParameter("gain")->setValueNotifyingHost(0.50f); processor.parameters.getParameter("peak")->setValueNotifyingHost(0.25f); }
            if (id == 2) { processor.unloadNamFile(); status.setText(processor.getNamStatus(), juce::dontSendNotification); repaint(); }
        });
    };
    addHitButton(topMeterSwitch, "top meter mode");
    topMeterSwitch.setLookAndFeel(&switchLookAndFeel);
    topMeterSwitch.setClickingTogglesState(true);
    topMeterSwitch.setTooltip("UP: OUTPUT | DOWN: GAIN REDUCTION");
    topMeterSwitch.onClick = [this] { cycleMeter(); };
    topMeterSwitch.onLeverDragState = [this](bool upper) {
        const int index = upper ? 1 : 2;
        processor.parameters.getParameter("meter")->setValueNotifyingHost(static_cast<float>(index) / 2.0f);
        meterSwitch.setToggleState(upper, juce::dontSendNotification);
        topMeterSwitch.setToggleState(upper, juce::dontSendNotification);
    };

    addHitButton(meterSwitch, "meter mode");
    meterSwitch.setLookAndFeel(&switchLookAndFeel);
    meterSwitch.setClickingTogglesState(true);
    meterSwitch.setTooltip("Pull UP: OUTPUT | Pull DOWN: GAIN REDUCTION");
    meterSwitch.onClick = [this] { cycleMeter(); };
    meterSwitch.onLeverDragState = [this](bool upper) {
        const int index = upper ? 1 : 2;
        processor.parameters.getParameter("meter")->setValueNotifyingHost(static_cast<float>(index) / 2.0f);
        meterSwitch.setToggleState(upper, juce::dontSendNotification);
        topMeterSwitch.setToggleState(upper, juce::dontSendNotification);
    };
    addHitButton(bottomPower, "bottom power switch");
    bottomPower.setLookAndFeel(&switchLookAndFeel);
    bottomPower.setClickingTogglesState(true);
    bottomPower.setTooltip("Pull UP: ON | Pull DOWN: OFF");
    bottomPower.onClick = [this] { setPower(!powerOn); bottomPower.setToggleState(powerOn, juce::dontSendNotification); };
    bottomPower.onLeverDragState = [this](bool upper) { setPower(upper); bottomPower.setToggleState(upper, juce::dontSendNotification); };
    addHitButton(limitSwitch, "limit compress switch");
    limitSwitch.setLookAndFeel(&switchLookAndFeel);
    limitSwitch.setClickingTogglesState(true);
    limitSwitch.setTooltip("Pull UP: LIMIT | Pull DOWN: COMPRESS");
    limitSwitch.onClick = [this] { setLimit(!limitOn); limitSwitch.setToggleState(limitOn, juce::dontSendNotification); };
    limitSwitch.onLeverDragState = [this](bool upper) { setLimit(upper); limitSwitch.setToggleState(upper, juce::dontSendNotification); };

    addAndMakeVisible(status); status.setColour(juce::Label::textColourId, juce::Colours::white);
    status.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    status.setJustificationType(juce::Justification::centred);
    status.setFont(juce::Font(juce::FontOptions(11.0f)));
    status.setMinimumHorizontalScale(0.65f);
    status.setText(processor.getNamStatus(), juce::dontSendNotification);
    addAndMakeVisible(switchStatus);
    // Live numeric readouts are separate labels so the knob face remains clean.
    for (auto* valueLabel : { &gainValueLabel, &peakValueLabel }) {
        addAndMakeVisible(*valueLabel);
        valueLabel->setJustificationType(juce::Justification::centred);
        // Numeric readout: black text, transparent background, and direct editing.
        // A single click opens the inline editor; Enter commits the value to the slider/APVTS.
        valueLabel->setColour(juce::Label::textColourId, juce::Colours::black);
        valueLabel->setColour(juce::Label::textWhenEditingColourId, juce::Colours::black);
        valueLabel->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        valueLabel->setColour(juce::Label::backgroundWhenEditingColourId, juce::Colours::white.withAlpha(0.92f));
        valueLabel->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        valueLabel->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::darkgrey);
        valueLabel->setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        valueLabel->setEditable(true, true, false);
        // JUCE Label::onEditorShow is a no-argument callback.
        // Retrieve the active editor from the Label instead of accepting an argument.
        valueLabel->onEditorShow = [valueLabel]() {
            if (auto* editor = valueLabel->getCurrentTextEditor())
                editor->selectAll();
        };
        valueLabel->onTextChange = [this, valueLabel]()
        {
            const auto text = valueLabel->getText().trim();
            if (text.isEmpty() || !text.containsOnly("+-0123456789.eE"))
                return;
            const double parsed = text.getDoubleValue();
            if (std::isfinite(parsed))
            {
                auto& target = (valueLabel == &gainValueLabel) ? gain : peak;
                target.setValue(juce::jlimit(target.getMinimum(), target.getMaximum(), parsed),
                                juce::sendNotificationSync);
            }
        };
    }
    switchStatus.setColour(juce::Label::textColourId, juce::Colours::white);
    switchStatus.setColour(juce::Label::backgroundColourId, juce::Colour(0xdd171717));
    switchStatus.setJustificationType(juce::Justification::centred);
    switchStatus.setFont(juce::Font(juce::FontOptions(12.0f)));
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    saveSnapshot(0); saveSnapshot(1);
    smoothedVuDb = 0.0f;
    startTimerHz(60);
}

ArmNongMuangLA2AAudioProcessorEditor::~ArmNongMuangLA2AAudioProcessorEditor() {
    // Reset each component explicitly. A mixed initializer_list of TextButton*
    // and LeverButton* cannot deduce one auto* element type on MSVC (C3535/C2440).
    topPower.setLookAndFeel(nullptr);
    preset.setLookAndFeel(nullptr);
    modelPicker.setLookAndFeel(nullptr);
    buttonA.setLookAndFeel(nullptr);
    buttonB.setLookAndFeel(nullptr);
    copyAB.setLookAndFeel(nullptr);
    settings.setLookAndFeel(nullptr);
    topMeterSwitch.setLookAndFeel(nullptr);
    meterSwitch.setLookAndFeel(nullptr);
    bottomPower.setLookAndFeel(nullptr);
    limitSwitch.setLookAndFeel(nullptr);
    gain.setLookAndFeel(nullptr);
    peak.setLookAndFeel(nullptr);
    attack.setLookAndFeel(nullptr);
    release.setLookAndFeel(nullptr);
}

void ArmNongMuangLA2AAudioProcessorEditor::NumericRotarySlider::mouseDoubleClick(const juce::MouseEvent& event) {
    juce::ignoreUnused(event);
    // Hardware-style reset: double-click returns the control to its documented default.
    const double defaultValue = getName() == "GAIN" ? 50.0 : 25.0;
    setValue(defaultValue, juce::sendNotificationSync);
}

void ArmNongMuangLA2AAudioProcessorEditor::setupRotary(juce::Slider& slider, const juce::String& name) {
    slider.setName(name); slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    // FIXED62: calibrated faceplate scale: 0=128, 50=260, 100=418 degrees.
    // The visual pointer uses piecewise mapping; endpoints define the drag sweep.
    slider.setRotaryParameters(juce::degreesToRadians(128.0f),
                               juce::degreesToRadians(418.0f), true);
    // Increase drag distance for finer, smoother control and less jumpy movement.
    slider.setMouseDragSensitivity(420);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&pointerLookAndFeel);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
}
void ArmNongMuangLA2AAudioProcessorEditor::addHitButton(juce::TextButton& b, const juce::String& name) {
    b.setName(name); b.setButtonText(juce::String{}); b.setLookAndFeel(&invisibleLookAndFeel);
    b.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    addAndMakeVisible(b);
}
void ArmNongMuangLA2AAudioProcessorEditor::SwitchLookAndFeel::drawButtonBackground(
    juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) {
    // Intentionally draw nothing. The original switches are part of the supplied
    // PNG faceplate. These transparent hit zones only provide mouse interaction;
    // they must never paint replacement lever artwork over the PNG.
}

void ArmNongMuangLA2AAudioProcessorEditor::PointerLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h, float pos,
    float startAngle, float endAngle, juce::Slider& slider) {
    juce::ignoreUnused(slider);

    // FIXED65: PNG-based two-layer hardware rendering.
    // The outer bezel is drawn directly from LA2A_Knob_Outer_Static.png,
    // unchanged and fixed; only the inner rotor PNG + pointer rotates.
    static const juce::Image outerStatic = juce::ImageCache::getFromMemory(
        BinaryData::LA2A_Knob_Outer_Static_png,
        BinaryData::LA2A_Knob_Outer_Static_pngSize);
    static const juce::Image innerRotor = juce::ImageCache::getFromMemory(
        BinaryData::LA2A_Knob_Inner_Rotor_png,
        BinaryData::LA2A_Knob_Inner_Rotor_pngSize);

    const float side = static_cast<float>(juce::jmin(w, h));
    const float cx = static_cast<float>(x) + static_cast<float>(w) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(h) * 0.5f;
    // The circular machined housing and rotor share the original image pivot.
    // Do not use the whole-image alpha bounding box here: the off-centre pointer
    // would bias that box and make the center cap miss the printed scale centre.
    const juce::Rectangle<float> destination(
        cx - side * 0.5f, cy - side * 0.5f, side, side);

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    if (outerStatic.isValid()) {
        // FIXED51 v5: reduce the fixed outer bezel to 50% while keeping it
        // centered on the rotor pivot. The inner rotor retains its full size.
        constexpr float outerRingScale = 0.50f;
        const float outerSide = side * outerRingScale;
        const juce::Rectangle<float> outerDestination(
            cx - outerSide * 0.5f, cy - outerSide * 0.5f,
            outerSide, outerSide);
        g.drawImage(outerStatic, outerDestination); // smaller and never rotates
    }

    if (innerRotor.isValid()) {
        // FIXED63: piecewise calibration from the real normalized APVTS/host value.
        // 0% = 128 degrees (lower-left), 50% = 263 degrees (top midpoint),
        // 100% = 418 degrees (lower-right). The midpoint is explicitly pinned.
        constexpr float sourcePointerAngleDegrees = 205.0f; // pointer orientation in source PNG
        const float normalized = juce::jlimit(0.0f, 1.0f, pos);
        const float targetAngleDegrees = (normalized <= 0.5f)
            ? (128.0f + (normalized / 0.5f) * (263.0f - 128.0f))
            : (263.0f + ((normalized - 0.5f) / 0.5f) * (418.0f - 263.0f));
        const float sourcePointerAngle = juce::degreesToRadians(sourcePointerAngleDegrees);
        const float parameterAngle = juce::degreesToRadians(targetAngleDegrees);
        const float rotation = parameterAngle - sourcePointerAngle;
        // FIXED63: keep the inner rotor pivot centered; do not skew or offset X/Y.

        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(rotation, cx, cy));
        g.drawImage(innerRotor, destination);
        g.restoreState();
    }
}

void ArmNongMuangLA2AAudioProcessorEditor::setPower(bool on) {
    powerOn = on; processor.parameters.getParameter("power")->setValueNotifyingHost(on ? 1.0f : 0.0f); repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::setLimit(bool on) {
    limitOn = on; processor.parameters.getParameter("limit")->setValueNotifyingHost(on ? 1.0f : 0.0f); repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::cycleMeter() {
    // Two-position LA-2A meter switch: DOWN = Gain Reduction, UP = Output.
    // AudioParameterChoice stores choice indices as normalized values: index / (numChoices - 1).
    const float normalized = processor.parameters.getParameter("meter")->getValue();
    const int currentIndex = juce::jlimit(0, 2, juce::roundToInt(normalized * 2.0f));
    const int nextIndex = (currentIndex == 1) ? 2 : 1;
    processor.parameters.getParameter("meter")->setValueNotifyingHost(static_cast<float>(nextIndex) / 2.0f);
    meterSwitch.setToggleState(nextIndex == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(nextIndex == 1, juce::dontSendNotification);
    repaint();
}
void ArmNongMuangLA2AAudioProcessorEditor::loadModelDialog() {
    chooser = std::make_unique<juce::FileChooser>(
        "Choose model file (.nam or .gen)", juce::File{}, "*.nam;*.gen");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (!file.existsAsFile()) return;

            juce::String message;
            const bool loaded = processor.loadNamFile(file, message);
            status.setText(message, juce::dontSendNotification);
            // Keep a successful model filename clearly visible in white.
            // Errors remain red for quick diagnosis.
            status.setColour(juce::Label::textColourId,
                             loaded ? juce::Colours::white : juce::Colour(0xffff5555));
            status.setTooltip(file.getFullPathName());
            repaint();
        });
}
void ArmNongMuangLA2AAudioProcessorEditor::saveSnapshot(int slot) {
    auto state = processor.parameters.copyState();
    if (slot == 0) snapshotA = state.createCopy(); else snapshotB = state.createCopy();
}
void ArmNongMuangLA2AAudioProcessorEditor::restoreSnapshot(int slot) {
    const auto& state = (slot == 0 ? snapshotA : snapshotB);
    if (state.isValid()) processor.parameters.replaceState(state.createCopy());
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
}
void ArmNongMuangLA2AAudioProcessorEditor::timerCallback() {
    // Smooth the meter needle with an exponential follower at 60 Hz.
    const float targetVuDb = juce::jlimit(-20.0f, 3.0f, processor.getVuValueDb());
    smoothedVuDb += (targetVuDb - smoothedVuDb) * 0.22f;
    if (std::abs(targetVuDb - smoothedVuDb) < 0.025f) smoothedVuDb = targetVuDb;
    powerOn = processor.parameters.getRawParameterValue("power")->load() >= 0.5f;
    limitOn = processor.parameters.getRawParameterValue("limit")->load() >= 0.5f;
    bottomPower.setToggleState(powerOn, juce::dontSendNotification);
    limitSwitch.setToggleState(limitOn, juce::dontSendNotification);
    meterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    topMeterSwitch.setToggleState(processor.getMeterMode() == 1, juce::dontSendNotification);
    const auto meterMode = processor.getMeterMode();
    // Do not overwrite a value while its inline editor is active.
    if (!gainValueLabel.isBeingEdited())
        gainValueLabel.setText(juce::String(gain.getValue(), 1), juce::dontSendNotification);
    if (!peakValueLabel.isBeingEdited())
        peakValueLabel.setText(juce::String(peak.getValue(), 1), juce::dontSendNotification);
    switchStatus.setText("POWER " + juce::String(powerOn ? "ON" : "OFF")
        + "   |   " + juce::String(limitOn ? "LIMIT" : "COMPRESS")
        + "   |   METER " + juce::String(meterMode == 1 ? "OUTPUT" : (meterMode == 2 ? "GAIN REDUCTION" : "INPUT")),
        juce::dontSendNotification);
    repaint();
}

void ArmNongMuangLA2AAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
    if (background.isValid()) g.drawImage(background, getLocalBounds().toFloat());

    const float sx = getWidth() / 1498.0f;
    const float sy = getHeight() / 775.0f;
    // Animated LA-2A-style VU needle, drawn over the PNG meter.
    const float db = juce::jlimit(-20.0f, 3.0f, smoothedVuDb);
    const float norm = (db + 20.0f) / 23.0f;
    const float angle = juce::degreesToRadians(-151.0f + norm * 107.0f);
    const float cx = 749.0f * sx, cy = 362.0f * sy, radius = 145.0f * sx;
    const float ex = cx + std::cos(angle) * radius;
    const float ey = cy + std::sin(angle) * radius;
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.drawLine(cx + sx, cy + sy, ex + sx, ey + sy, 4.0f * sx);
    g.setColour(juce::Colour(0xff21170b));
    g.drawLine(cx, cy, ex, ey, 2.5f * sx);
    g.setColour(juce::Colour(0xffd0b16d));
    g.fillEllipse(cx - 4.0f*sx, cy - 4.0f*sy, 8.0f*sx, 8.0f*sy);
    g.setColour(juce::Colour(0xff342617));
    g.drawEllipse(cx - 4.0f*sx, cy - 4.0f*sy, 8.0f*sx, 8.0f*sy, 1.0f*sx);
    modelPicker.setTooltip(processor.isNamLoaded()
        ? "Loaded model: " + processor.getLoadedNamName()
        : "Click to load a compatible .nam or .gen model. Built-in compressor remains available.");
}

void ArmNongMuangLA2AAudioProcessorEditor::resized() {
    const float sx = getWidth() / 1498.0f, sy = getHeight() / 775.0f;
    auto place = [&](juce::Component& c, float x, float y, float w, float h) {
        c.setBounds(juce::roundToInt(x*sx), juce::roundToInt(y*sy),
                    juce::roundToInt(w*sx), juce::roundToInt(h*sy));
    };
    // Coordinates are measured directly against the new 1498 x 775 PNG.
    // The original 1498x775 faceplate pivot centres are exactly (290, 330)
    // and (1208, 330). A 205px slider area enlarges the visible 2048px PNG
    // hardware by about 8% over FIXED56 while keeping its alpha silhouette
    // inside the printed tick scale. The bitmap alpha-centre correction above
    // aligns the visible metal assembly to these exact faceplate pivots.
    constexpr float knobSide = 205.0f;
    constexpr float knobShiftRight = -8.0f; // moved 20 px left from v5
    // Move each complete control (hit area + fixed outer ring + inner rotor)
    // together so the PNG layers remain perfectly registered.
    place(gain, 290.0f + knobShiftRight - knobSide * 0.5f,
          330.0f - knobSide * 0.5f, knobSide, knobSide);
    place(peak, 1208.0f + knobShiftRight - knobSide * 0.5f,
          330.0f - knobSide * 0.5f, knobSide, knobSide);
    place(gainValueLabel, 190, 430, 160, 29);
    place(peakValueLabel, 1107, 430, 160, 29);
    place(attack, 0, 0, 1, 1);
    place(release, 0, 0, 1, 1);

    // Top strip: clickable zones follow the printed controls in the PNG.
    place(topPower, 80, 48, 64, 76);
    place(preset, 226, 54, 310, 57);
    place(modelPicker, 584, 54, 312, 57);
    place(buttonA, 946, 50, 54, 72);
    place(buttonB, 1014, 50, 54, 72);
    place(copyAB, 1096, 54, 70, 60);
    place(settings, 1215, 54, 62, 60);
    place(topMeterSwitch, 1335, 49, 38, 64);

    // Bottom lever switches: UP=LIMIT/ON/OUTPUT, DOWN=COMPRESS/OFF/GAIN REDUCTION.
    place(limitSwitch, 338, 638, 46, 78);
    place(bottomPower, 737, 638, 46, 78);
    place(meterSwitch, 1195, 638, 46, 78);
    // Visible model status/filename line under the model-load hit area.
    place(status, 584, 113, 312, 26);
    place(switchStatus, 520, 730, 458, 28);
}
