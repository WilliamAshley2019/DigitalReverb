// PluginEditor.cpp - Fixed with Resizable Support and Custom Styles
#include "PluginEditor.h"

//==============================================================================
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    DBG("=== PluginEditor Constructor START ===");

    // Set initial size and enable resizing (JUCE 8 style)
    setSize(900, 550);
    setResizable(true, true);
    setResizeLimits(700, 450, 1200, 750);

    // Main LCD first
    addAndMakeVisible(mainLcd);

    // Initialize all knobs
    for (size_t i = 0; i < parameterKnobs.size(); ++i)
    {
        parameterKnobs[i] = std::make_unique<ParameterKnobWithLcd>();
        addAndMakeVisible(*parameterKnobs[i]);
        parameterKnobs[i]->setVisible(true);
    }

    // Initialize all knob labels
    for (size_t i = 0; i < knobLabels.size(); ++i)
    {
        addAndMakeVisible(knobLabels[i]);
        knobLabels[i].setJustificationType(juce::Justification::centred);
        knobLabels[i].setFont(juce::FontOptions(12.0f, juce::Font::bold));
        knobLabels[i].setColour(juce::Label::textColourId, juce::Colours::silver);
    }

    if (auto* module = audioProcessor.getEffectModule())
    {
        mainLcd.setText(module->getModuleName().toUpperCase(), 0);
        mainLcd.setText(module->getModuleDescription(), 1);

        const auto paramDefs = module->getParameterDefinitions();

        // Map parameters to knobs
        for (size_t i = 0; i < parameterKnobs.size(); ++i)
        {
            if (i < paramDefs.size() && parameterKnobs[i])
            {
                parameterKnobs[i]->setParameter(paramDefs[i]);
                parameterKnobs[i]->attachToParameter(audioProcessor.getParameters(),
                    "param" + juce::String(static_cast<int>(i)));

                knobLabels[i].setText(paramDefs[i].label, juce::dontSendNotification);
            }
            else if (parameterKnobs[i])
            {
                parameterKnobs[i]->setVisible(false);
                knobLabels[i].setText("", juce::dontSendNotification);
            }
        }

        // Preset selector
        addAndMakeVisible(presetLabel);
        presetLabel.setText("PRESET:", juce::dontSendNotification);
        presetLabel.setJustificationType(juce::Justification::right);
        presetLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        presetLabel.setColour(juce::Label::textColourId, juce::Colours::silver);

        addAndMakeVisible(presetSelector);
        presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(60, 60, 65));
        presetSelector.setColour(juce::ComboBox::textColourId, juce::Colours::silver);
        presetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);

        const auto presets = module->getFactoryPresets();

        for (size_t i = 0; i < presets.size(); ++i)
        {
            presetSelector.addItem(presets[i].name, static_cast<int>(i) + 1);
        }

        presetSelector.onChange = [this, module, presets]
            {
                const int index = presetSelector.getSelectedItemIndex();
                if (index >= 0 && index < static_cast<int>(presets.size()))
                {
                    audioProcessor.loadPreset(presets[static_cast<size_t>(index)]);
                    mainLcd.setText("LOADED: " + presets[static_cast<size_t>(index)].name, 2);

                    // Force update of all knobs
                    for (auto& knob : parameterKnobs)
                    {
                        if (knob)
                            knob->updateDisplay();
                    }
                }
            };

        // Select first preset by default
        if (presets.size() > 0)
        {
            presetSelector.setSelectedItemIndex(0, juce::dontSendNotification);
        }
    }

    constexpr int timerFrequencyHz = 30;
    startTimerHz(timerFrequencyHz);

    // CRITICAL FIX: Force initial layout by triggering resized()
    // This ensures knobs are visible immediately without needing to resize window
    resized();

    DBG("PluginEditor constructed with " << parameterKnobs.size() << " knobs");
    DBG("=== PluginEditor Constructor END ===");
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

//==============================================================================
void PluginEditor::paint(juce::Graphics& g)
{
    // Dark background first
    g.fillAll(juce::Colour(30, 30, 35));

    // Metal texture gradient (original silver tones)
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(70, 70, 75), 0.0f, 0.0f,
        juce::Colour(50, 50, 55), 0.0f, static_cast<float>(getHeight()), false));
    g.fillRect(getLocalBounds());

    // Top label
    g.setFont(juce::FontOptions("Arial", 20.0f, juce::Font::bold));
    g.setColour(juce::Colours::silver);
    g.drawText("WXYZ Digital 256", 0, 10, getWidth(), 30,
        juce::Justification::centred);

    // Subtitle
    g.setFont(juce::FontOptions("Arial", 12.0f, juce::Font::plain));
    g.drawText("Reverb", 0, 35, getWidth(), 20,
        juce::Justification::centred);

    // Section dividers
    g.setColour(juce::Colour(90, 90, 95));
    g.drawLine(20.0f, 150.0f, static_cast<float>(getWidth() - 20), 150.0f, 2.0f);
    g.drawLine(20.0f, 320.0f, static_cast<float>(getWidth() - 20), 320.0f, 2.0f);
}

//==============================================================================
void PluginEditor::resized()
{
    debugLayout();

    constexpr int margin = 20;
    constexpr int topSectionHeight = 60;
    constexpr int lcdHeight = 100;
    constexpr int lcdHorizontalPadding = 100;
    constexpr int lcdVerticalPadding = 10;
    constexpr int presetSectionHeight = 40;
    constexpr int presetLabelWidth = 80;
    constexpr int presetSelectorWidth = 200;

    // Knob section - position at bottom
    constexpr int knobSectionTop = 300;
    constexpr int knobLabelHeight = 25;
    constexpr int knobSpacing = 10;
    const int knobHeight = 150;

    auto area = getLocalBounds();

    // Title area (already painted)
    auto titleArea = area.removeFromTop(topSectionHeight);

    // LCD display
    auto lcdArea = area.removeFromTop(lcdHeight);
    mainLcd.setBounds(lcdArea.reduced(lcdHorizontalPadding, lcdVerticalPadding));

    // Preset selector
    auto presetArea = area.removeFromTop(presetSectionHeight);
    presetArea = presetArea.withSizeKeepingCentre(300, presetSectionHeight);
    presetLabel.setBounds(presetArea.removeFromLeft(presetLabelWidth));
    presetSelector.setBounds(presetArea.removeFromLeft(presetSelectorWidth).reduced(5));

    // Calculate knob positions
    const int knobCount = static_cast<int>(parameterKnobs.size());
    const int totalWidth = getWidth() - 2 * margin;
    const int availableWidthForKnobs = totalWidth - (knobCount - 1) * knobSpacing;
    const int knobWidth = availableWidthForKnobs / knobCount;

    // Position knob labels
    for (int i = 0; i < knobCount; ++i)
    {
        int labelX = margin + i * (knobWidth + knobSpacing);
        knobLabels[i].setBounds(labelX, knobSectionTop, knobWidth, knobLabelHeight);
    }

    // Position knobs
    int knobY = knobSectionTop + knobLabelHeight + 5;
    for (int i = 0; i < knobCount; ++i)
    {
        if (parameterKnobs[static_cast<size_t>(i)])
        {
            int knobX = margin + i * (knobWidth + knobSpacing);
            juce::Rectangle<int> knobBounds(knobX, knobY, knobWidth, knobHeight);
            parameterKnobs[static_cast<size_t>(i)]->setBounds(knobBounds);
            parameterKnobs[static_cast<size_t>(i)]->setVisible(true);
            parameterKnobs[static_cast<size_t>(i)]->repaint();
        }
    }
}

//==============================================================================
void PluginEditor::timerCallback()
{
    updateMainLcd();
}

void PluginEditor::updateMainLcd()
{
    auto* module = audioProcessor.getEffectModule();
    if (module == nullptr || !mainLcd.isLcdEnabled())
        return;

    if (module->hasRealtimeDisplay())
    {
        const juce::String info = module->getRealtimeDisplayInfo();
        mainLcd.setText(info, 3);
    }
}

//==============================================================================
void PluginEditor::debugLayout()
{
    DBG("=== PluginEditor Layout Debug ===");
    DBG("Window size: " << getWidth() << "x" << getHeight());
    DBG("Number of knobs: " << parameterKnobs.size());

    int visibleKnobs = 0;
    for (size_t i = 0; i < parameterKnobs.size(); ++i)
    {
        if (parameterKnobs[i] && parameterKnobs[i]->isVisible())
        {
            visibleKnobs++;
            auto bounds = parameterKnobs[i]->getBounds();
            DBG("Knob " << i << " bounds: "
                << bounds.getX() << "," << bounds.getY() << " "
                << bounds.getWidth() << "x" << bounds.getHeight());
        }
    }
    DBG("Visible knobs: " << visibleKnobs);
}