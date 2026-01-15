// EffectModule.h - Base interface for all effect modules
#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

// Forward declarations
class DelayMemoryPool;
class FixedPointEngine;
struct FixedPointSample;

//==============================================================================
// Parameter Information Structure
//==============================================================================
struct EffectParameter {
    juce::String id;
    juce::String name;
    juce::String label;      // Short label for small LCD (e.g., "DECAY")
    juce::String unit;       // Unit string (e.g., "ms", "%", "dB")
    float minValue;
    float maxValue;
    float defaultValue;
    float stepSize;
    bool isLogarithmic;

    EffectParameter(const juce::String& paramId,
        const juce::String& paramName,
        const juce::String& paramLabel,
        const juce::String& paramUnit,
        float min, float max, float defaultVal, float step = 0.01f,
        bool logarithmic = false)
        : id(paramId), name(paramName), label(paramLabel), unit(paramUnit),
        minValue(min), maxValue(max), defaultValue(defaultVal),
        stepSize(step), isLogarithmic(logarithmic)
    {
    }
};

//==============================================================================
// Preset Structure
//==============================================================================
struct EffectPreset {
    juce::String name;
    juce::String description;
    std::vector<float> parameterValues;

    EffectPreset(const juce::String& presetName,
        const juce::String& desc,
        const std::vector<float>& values)
        : name(presetName), description(desc), parameterValues(values)
    {
    }
};

//==============================================================================
// Base Effect Module Interface
//==============================================================================
class EffectModule {
public:
    virtual ~EffectModule() = default;

    // Module identification
    virtual juce::String getModuleName() const = 0;
    virtual juce::String getModuleDescription() const = 0;
    virtual int getModuleVersion() const { return 1; }

    // Parameter configuration
    virtual std::vector<EffectParameter> getParameterDefinitions() const = 0;
    virtual int getParameterCount() const = 0;

    // Preset management
    virtual std::vector<EffectPreset> getFactoryPresets() const = 0;
    virtual void loadPreset(const EffectPreset& preset) = 0;
    virtual EffectPreset getCurrentPreset() const = 0;

    // DSP lifecycle
    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual void reset() = 0;
    virtual void releaseResources() = 0;

    // Parameter updates
    virtual void setParameter(int parameterIndex, float value) = 0;
    virtual float getParameter(int parameterIndex) const = 0;
    virtual juce::String getParameterDisplay(int parameterIndex) const = 0;

    // Audio processing
    virtual void process(FixedPointSample& left, FixedPointSample& right,
        DelayMemoryPool& delayPool, FixedPointEngine& dspCore) = 0;

    // Modulation updates (called at control rate, e.g., every 64 samples)
    virtual void updateModulation(int blockCounter) { juce::ignoreUnused(blockCounter); }

    // Real-time info for LCD display (optional)
    virtual bool hasRealtimeDisplay() const { return false; }
    virtual juce::String getRealtimeDisplayInfo() const { return ""; }

protected:
    double sampleRate = 44100.0;
    int blockSize = 512;
};

//==============================================================================
// Factory function type for creating effect modules
// This will allow easy registration of new effects in the multi-FX version
//==============================================================================
using EffectModuleFactory = std::function<std::unique_ptr<EffectModule>()>;

//==============================================================================
// Effect Module Registry (for future multi-FX version)
// Currently commented out but ready to use
//==============================================================================
/*
class EffectModuleRegistry {
public:
    static EffectModuleRegistry& getInstance() {
        static EffectModuleRegistry instance;
        return instance;
    }

    void registerModule(const juce::String& moduleName, EffectModuleFactory factory) {
        modules[moduleName] = factory;
    }

    std::unique_ptr<EffectModule> createModule(const juce::String& moduleName) {
        auto it = modules.find(moduleName);
        if (it != modules.end())
            return it->second();
        return nullptr;
    }

    juce::StringArray getAvailableModules() const {
        juce::StringArray names;
        for (const auto& pair : modules)
            names.add(pair.first);
        return names;
    }

private:
    std::map<juce::String, EffectModuleFactory> modules;
    EffectModuleRegistry() = default;
};
*/