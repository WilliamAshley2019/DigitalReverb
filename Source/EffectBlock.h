// EffectBlock.h - Complete base effect class
#pragma once

#include <JuceHeader.h>

// Forward declarations
struct FixedPointSample;
class DelayMemoryPool;
class FixedPointEngine;

//==============================================================================
// Fixed-Point Sample Type
//==============================================================================
struct FixedPointSample {
    int32_t value = 0;

    static constexpr int32_t Q23_MAX = 0x7FFFFF;
    static constexpr int32_t Q23_MIN = -0x800000;

    inline void saturate(int bitDepth = 24) {
        if (bitDepth == 16) {
            value = juce::jlimit<int32_t>(Q23_MIN >> 8, Q23_MAX >> 8, value >> 8) << 8;
        }
        else if (bitDepth == 12) {
            value = juce::jlimit<int32_t>(Q23_MIN >> 12, Q23_MAX >> 12, value >> 12) << 12;
        }
        else {
            value = juce::jlimit<int32_t>(Q23_MIN, Q23_MAX, value);
        }
    }
};

//==============================================================================
// Effect Block Base Class
//==============================================================================
class EffectBlock {
public:
    enum Type {
        OFF = 0,
        REVERB_HALL,
        REVERB_PLATE,
        REVERB_ROOM,
        REVERB_GATED,
        REVERB_REVERSE,
        DELAY_MONO,
        DELAY_STEREO,
        DELAY_PINGPONG,
        DELAY_TAPE,
        CHORUS,
        FLANGER,
        PITCH_SHIFT,
        PARAMETRIC_EQ,
        GRAPHIC_EQ,
        PHASER,
        TREMOLO,
        ROTARY,
        COMPRESSOR,
        LIMITER,
        NOISE_GATE,
        DISTORTION,
        FILTER_LPF,
        FILTER_HPF,
        TOTAL_EFFECTS
    };

    virtual ~EffectBlock() = default;

    virtual void process(FixedPointSample& left, FixedPointSample& right,
        DelayMemoryPool& pool, FixedPointEngine& dsp) = 0;

    virtual void updateModulation(int blockCounter) { juce::ignoreUnused(blockCounter); }
    virtual void setSampleRate(double sr) { sampleRate = sr; }
    virtual void setParam(int paramID, float value) = 0;

    static juce::String getEffectName(Type type);

protected:
    double sampleRate = 44100.0;
    float currentParams[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
};