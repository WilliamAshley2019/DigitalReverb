// ReverbHall.h - Hall Reverb Effect Module (CORRECTED - remove duplicate)
#pragma once

#include "EffectModule.h"
#include "FixedPointDSP.h"
#include "DelayMemoryPool.h"
#include <array>

//==============================================================================
// Reverb Hall Effect Module
// Based on Schroeder-Moorer reverb architecture with parallel comb filters
// and series allpass filters
//==============================================================================
class ReverbHall : public EffectModule {
public:
    ReverbHall();
    ~ReverbHall() override = default;

    // Module identification
    juce::String getModuleName() const override { return "Reverb Hall"; }
    juce::String getModuleDescription() const override {
        return "Classic hall reverb with warm, spacious character. "
            "Based on Schroeder-Moorer architecture.";
    }

    // Parameter configuration
    std::vector<EffectParameter> getParameterDefinitions() const override;
    int getParameterCount() const override { return 7; }

    // Preset management
    std::vector<EffectPreset> getFactoryPresets() const override;
    void loadPreset(const EffectPreset& preset) override;
    EffectPreset getCurrentPreset() const override;

    // DSP lifecycle
    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void releaseResources() override;

    // Parameter updates
    void setParameter(int parameterIndex, float value) override;
    float getParameter(int parameterIndex) const override;
    juce::String getParameterDisplay(int parameterIndex) const override;

    // Audio processing
    void process(FixedPointSample& left, FixedPointSample& right,
        DelayMemoryPool& delayPool, FixedPointEngine& dspCore) override;

    // Modulation updates (called at control rate)
    void updateModulation(int blockCounter) override;

    // Real-time display
    bool hasRealtimeDisplay() const override { return true; }
    juce::String getRealtimeDisplayInfo() const override;

private:
    // Parameters (0.0 to 1.0 normalized)
    float preDelay = 0.0f;        // 0-100ms (maps to 0-100ms)
    float decayTime = 0.7f;       // RT60: 0.1-10 seconds (logarithmic)
    float diffusion = 0.8f;       // 0-100% (density control)
    float damping = 0.5f;         // HF Damping: 0-100%
    float earlyLevel = 0.7f;      // Early Reflections: 0-100%
    float size = 1.0f;            // Room Size: 0.5-2.0x scaling
    float mix = 0.5f;             // Dry/Wet Mix: 0-100%

    // DSP State Structures
    struct CombFilter {
        int delayLength = 0;
        int32_t feedbackGain = 0;      // Q12 fixed-point
        std::vector<int32_t> buffer;   // Q12 fixed-point samples
        int writeIndex = 0;
    };

    struct AllpassFilter {
        int delayLength = 0;
        int32_t coeff = 0;             // Q12 fixed-point
        std::vector<int32_t> buffer;   // Q12 fixed-point samples
        int writeIndex = 0;
    };

    struct EarlyReflection {
        int delayLength = 0;
        int32_t gain = 0;              // Q12 fixed-point
        std::vector<int32_t> buffer;   // Q12 fixed-point samples
        int writeIndex = 0;
    };

    // Filter arrays
    std::array<CombFilter, 4> combFilters;
    std::array<AllpassFilter, 2> allpassFilters;
    std::array<EarlyReflection, 8> earlyReflections;

    // Pre-delay buffer (floats for simplicity)
    std::vector<float> preDelayBuffer;
    int preDelayWriteIndex = 0;
    int preDelayReadOffset = 0;

    // Damping filter states
    int32_t dampingAlpha = 0;          // Q12 fixed-point coefficient
    int32_t dampingStateL = 0;         // Q12 fixed-point state
    int32_t dampingStateR = 0;         // Q12 fixed-point state

    // Low-pass filter states for output (NOT static - per instance)
    float lpfStateL = 0.0f;
    float lpfStateR = 0.0f;

    // DC offset filter states
    float dcOffsetStateL = 0.0f;
    float dcOffsetStateR = 0.0f;

    // Modulation (subtle chorusing)
    float lfoPhase = 0.0f;
    float lfoRate = 0.5f;              // Hz
    float modulationDepth = 0.0f;

    // Real-time monitoring
    float currentTailLevel = 0.0f;
    float estimatedRT60 = 2.0f;

    // Sample rate and block size
    double sampleRate = 44100.0;
    int blockSize = 512;

    // Helper methods
    void updateParameters();
    float processCombFilter(CombFilter& comb, float input, FixedPointEngine& dsp);
    float processAllpassFilter(AllpassFilter& ap, float input, FixedPointEngine& dsp);

    // Fixed-point conversion helpers
    int32_t floatToFixed(float f);
    float fixedToFloat(int32_t fixed);

    // Buffer initialization helpers
    void initializeBuffers();

    // Parameter validation
    void validateParameters();
};