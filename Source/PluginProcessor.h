// PluginProcessor.h - Main Audio Processor
#pragma once

#include <JuceHeader.h>
#include "EffectModule.h"
#include "FixedPointDSP.h"
#include "DelayMemoryPool.h"

// For standalone build - include only the current effect
#include "ReverbHall.h"

// For multi-FX build (currently commented out):
// #include "ReverbPlate.h"
// #include "MonoDelay.h"
// ... etc

//==============================================================================
class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Parameter access
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Effect module access
    EffectModule* getEffectModule() const { return effectModule.get(); }

    // Preset management
    void loadPreset(const EffectPreset& preset);

private:
    juce::AudioProcessorValueTreeState parameters;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Core
    std::unique_ptr<DelayMemoryPool> delayPool;
    std::unique_ptr<FixedPointEngine> dspCore;

    // Effect module
    std::unique_ptr<EffectModule> effectModule;

    // Processing
    juce::CriticalSection processingLock;
    int modulationCounter = 0;
    static constexpr int MODULATION_UPDATE_RATE = 64; // Update every 64 samples

    void updateParametersFromModule();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};