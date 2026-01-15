// PluginEditor.h - Fixed with Resizable Support and Custom Styles
#pragma once

#include "PluginProcessor.h"
#include <array>

//==============================================================================
// Custom Hardware Knob Look and Feel
//==============================================================================
class HardwareKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
        juce::Slider& slider) override
    {
        juce::ignoreUnused(slider); // Suppress unused parameter warning

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // 1. Draw Knob Shadow
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillEllipse(bounds.translated(2, 3));

        // 2. Draw Main Black Body (with slight gradient for depth)
        juce::ColourGradient bodyGrad(juce::Colour(40, 40, 42), bounds.getCentreX(), bounds.getCentreY(),
            juce::Colour(15, 15, 17), bounds.getCentreX(), bounds.getBottom(), true);
        g.setGradientFill(bodyGrad);
        g.fillEllipse(bounds);

        // 3. Draw Brushed Metal Inner Cap
        auto capBounds = bounds.reduced(radius * 0.25f);
        juce::ColourGradient capGrad(juce::Colour(120, 120, 125), capBounds.getCentreX(), capBounds.getCentreY(),
            juce::Colour(60, 60, 65), capBounds.getCentreX(), capBounds.getBottom(), true);
        g.setGradientFill(capGrad);
        g.fillEllipse(capBounds);

        // 4. Draw Silver Indicator Line
        juce::Path p;
        auto pointerThickness = 3.5f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, radius * 0.5f);

        g.setColour(juce::Colours::silver);
        g.fillPath(p, juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));

        // 5. Draw Outer Chrome/Silver Ring
        g.setColour(juce::Colours::silver.withAlpha(0.4f));
        g.drawEllipse(bounds, 1.5f);
    }
};

//==============================================================================
// Small LCD Display Component (Hardware-style - Original DSP256 Style)
//==============================================================================
class SmallLcdDisplay final : public juce::Component
{
public:
    SmallLcdDisplay()
    {
        setSize(100, 40);
    }

    void paint(juce::Graphics& g) override
    {
        // LCD background (greenish backlight - original style)
        g.fillAll(juce::Colour(120, 140, 100));

        // LCD border
        g.setColour(juce::Colour(60, 70, 50));
        g.drawRect(getLocalBounds(), 2);

        // Dark text on light LCD background
        g.setColour(juce::Colour(20, 25, 15));
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
            12.0f, juce::Font::bold));
        g.drawText(label, 2, 2, getWidth() - 4, 16,
            juce::Justification::centred, false);

        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
            14.0f, juce::Font::plain));
        g.drawText(valueText, 2, 20, getWidth() - 4, 16,
            juce::Justification::centred, false);
    }

    void setLabel(const juce::String& text)
    {
        label = text;
        repaint();
    }

    void setValue(const juce::String& text)
    {
        if (valueText != text)
        {
            valueText = text;
            repaint();
        }
    }

private:
    juce::String label;
    juce::String valueText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmallLcdDisplay)
};

//==============================================================================
// Main LCD Display Component (Hardware-style - Original DSP256 Style)
//==============================================================================
class MainLcdDisplay final : public juce::Component
{
public:
    MainLcdDisplay()
    {
        setSize(500, 80);

        lcdOnButton.setButtonText("LCD");
        lcdOnButton.setToggleState(true, juce::dontSendNotification);
        lcdOnButton.setColour(juce::TextButton::buttonColourId,
            juce::Colour(60, 70, 50));
        lcdOnButton.setColour(juce::TextButton::textColourOffId,
            juce::Colour(200, 200, 180));

        lcdOnButton.onClick = [this]
            {
                lcdEnabled = lcdOnButton.getToggleState();
                repaint();
            };

        addAndMakeVisible(lcdOnButton);
    }

    void paint(juce::Graphics& g) override
    {
        if (lcdEnabled)
        {
            // LCD background (greenish backlight - original style)
            g.fillAll(juce::Colour(120, 140, 100));
            g.setColour(juce::Colour(60, 70, 50));
        }
        else
        {
            g.fillAll(juce::Colour(40, 50, 35));
            g.setColour(juce::Colour(30, 40, 25));
        }

        g.drawRect(getLocalBounds(), 2);

        if (!lcdEnabled)
            return;

        // Dark text on LCD background
        g.setColour(juce::Colour(20, 25, 15));
        g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),
            16.0f, juce::Font::bold));

        for (int i = 0; i < 4; ++i)
        {
            g.drawText(lines[i], 5, 2 + i * 18, getWidth() - 60, 18,
                juce::Justification::left, false);
        }
    }

    void resized() override
    {
        lcdOnButton.setBounds(getWidth() - 50, 5, 45, 20);
    }

    void setText(const juce::String& text, int line)
    {
        if (line >= 0 && line < 4 && lines[line] != text)
        {
            lines[line] = text;
            if (lcdEnabled)
                repaint();
        }
    }

    [[nodiscard]] bool isLcdEnabled() const noexcept { return lcdEnabled; }

private:
    juce::String lines[4];
    juce::TextButton lcdOnButton;
    bool lcdEnabled = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLcdDisplay)
};

//==============================================================================
// Parameter Knob with Small LCD - Hardware Style
//==============================================================================
class ParameterKnobWithLcd final : public juce::Component
{
public:
    ParameterKnobWithLcd()
    {
        // Setup slider with custom look and feel
        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
            juce::MathConstants<float>::pi * 2.75f, true);

        knob.setLookAndFeel(&hardwareLF);
        knob.setVelocityBasedMode(false);
        knob.setRange(0.0, 1.0, 0.01);
        knob.setValue(0.5);
        knob.setDoubleClickReturnValue(true, 0.5);

        knob.onValueChange = [this]
            {
                updateLcdValue();
            };

        addAndMakeVisible(knob);
        addAndMakeVisible(lcd);

        lcd.setLabel("PARAM");
        lcd.setValue("0.50");

        setVisible(true);
        knob.setVisible(true);
        lcd.setVisible(true);
    }

    ~ParameterKnobWithLcd() override
    {
        knob.setLookAndFeel(nullptr);
    }

    void setParameter(const EffectParameter& param)
    {
        parameterInfo = param;
        knob.setRange(param.minValue, param.maxValue, param.stepSize);
        knob.setValue(param.defaultValue, juce::dontSendNotification);
        lcd.setLabel(param.label);
        updateLcdValue();
    }

    void attachToParameter(juce::AudioProcessorValueTreeState& apvts,
        const juce::String& paramID)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramID, knob);
    }

    void resized() override
    {
        auto area = getLocalBounds();

        const int totalHeight = area.getHeight();
        const int lcdHeight = 45;
        const int knobHeight = totalHeight - lcdHeight;
        const int knobPadding = 5;

        auto knobArea = area.removeFromTop(knobHeight).reduced(knobPadding);
        int knobSize = juce::jmin(knobArea.getWidth(), knobArea.getHeight());
        knob.setBounds(knobArea.withSizeKeepingCentre(knobSize, knobSize));

        lcd.setBounds(area.withSizeKeepingCentre(90, 40));

        knob.repaint();
        lcd.repaint();
    }

    void updateDisplay()
    {
        updateLcdValue();
    }

private:
    juce::Slider knob;
    SmallLcdDisplay lcd;
    HardwareKnobLookAndFeel hardwareLF;
    EffectParameter parameterInfo{ "", "", "", "", 0.0f, 1.0f, 0.5f };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void updateLcdValue()
    {
        const float value = static_cast<float>(knob.getValue());
        juce::String display;

        if (parameterInfo.unit.isNotEmpty())
        {
            if (parameterInfo.unit == "ms")
                display = juce::String(value, 0) + "ms";
            else if (parameterInfo.unit == "%")
                display = juce::String(value, 0) + "%";
            else if (parameterInfo.unit == "s")
                display = juce::String(value, 1) + "s";
            else if (parameterInfo.unit == "x")
                display = juce::String(value, 2) + "x";
            else
                display = juce::String(value, 1) + parameterInfo.unit;
        }
        else
        {
            display = juce::String(value, 2);
        }

        lcd.setValue(display);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterKnobWithLcd)
};

//==============================================================================
// Main Plugin Editor Class - RESIZABLE
//==============================================================================
class PluginEditor final : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateMainLcd();
    void debugLayout();

    PluginProcessor& audioProcessor;
    MainLcdDisplay mainLcd;

    std::array<std::unique_ptr<ParameterKnobWithLcd>, 7> parameterKnobs;
    std::array<juce::Label, 7> knobLabels;

    juce::ComboBox presetSelector;
    juce::Label presetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};