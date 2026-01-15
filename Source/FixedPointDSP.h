// FixedPointDSP.h - Corrected with Q12 method names
#pragma once

#include <JuceHeader.h>

//==============================================================================
// HISC-style Fixed-Point (20-bit with 12-bit fraction)
// Similar to Motorola DSP56k architecture
//==============================================================================
struct FixedPointSample {
    int32_t value = 0;

    // 20-bit range for HISC DSP (-524288 to 524287)
    static constexpr int32_t HISC_MAX = 0x7FFFF;   // 524287
    static constexpr int32_t HISC_MIN = -0x80000;  // -524288

    // Q12 format (12 fractional bits)
    static constexpr int32_t Q12_ONE = 1 << 12;  // 4096

    inline void saturate() {
        value = juce::jlimit<int32_t>(HISC_MIN, HISC_MAX, value);
    }
};

//==============================================================================
// Fixed-Point DSP Engine (HISC Emulation)
//==============================================================================
class FixedPointEngine {
public:
    FixedPointEngine() = default;

    // Prepare (standard JUCE interface)
    void prepare(double sr) {
        this->sampleRate = sr;
        dcOffsetFilterCoeff = 0.999f;
        dcOffsetStateL = 0.0f;
        dcOffsetStateR = 0.0f;
    }

    // HISC-style MAC operation (Multiply-Accumulate)
    FixedPointSample mac(FixedPointSample a, FixedPointSample b, FixedPointSample c, bool& overflow) {
        int64_t product = static_cast<int64_t>(a.value) * b.value;
        int64_t result = (product >> 12) + static_cast<int64_t>(c.value);

        overflow = (result > static_cast<int64_t>(FixedPointSample::HISC_MAX) ||
            result < static_cast<int64_t>(FixedPointSample::HISC_MIN));

        FixedPointSample output;
        output.value = static_cast<int32_t>(juce::jlimit<int64_t>(
            static_cast<int64_t>(FixedPointSample::HISC_MIN),
            static_cast<int64_t>(FixedPointSample::HISC_MAX),
            result));
        return output;
    }

    // Multiply with overflow detection
    FixedPointSample multiply(FixedPointSample a, FixedPointSample b, bool& overflow) {
        int64_t product = static_cast<int64_t>(a.value) * b.value;
        product >>= 12; // Q12 * Q12 -> Q12

        overflow = (product > static_cast<int64_t>(FixedPointSample::HISC_MAX) ||
            product < static_cast<int64_t>(FixedPointSample::HISC_MIN));

        FixedPointSample result;
        result.value = static_cast<int32_t>(juce::jlimit<int64_t>(
            static_cast<int64_t>(FixedPointSample::HISC_MIN),
            static_cast<int64_t>(FixedPointSample::HISC_MAX),
            product));
        return result;
    }

    // Float to HISC 20-bit fixed-point (Updated to Q12 naming)
    FixedPointSample floatToQ12(float f) {
        FixedPointSample result;
        // Scale to Q12 (12 fractional bits)
        float scaled = f * static_cast<float>(FixedPointSample::Q12_ONE);
        result.value = static_cast<int32_t>(scaled);
        result.saturate();
        return result;
    }

    // HISC to float conversion (Updated to Q12 naming)
    float Q12ToFloat(FixedPointSample q) {
        return static_cast<float>(q.value) / static_cast<float>(FixedPointSample::Q12_ONE);
    }

    // Apply DC offset filter (common in vintage DSP)
    FixedPointSample dcBlock(FixedPointSample input, float& state) {
        float inputFloat = Q12ToFloat(input);
        float output = inputFloat - state + dcOffsetFilterCoeff * state;
        state = inputFloat;
        return floatToQ12(output);
    }

    // Simple multiply without overflow check
    FixedPointSample multiplySimple(FixedPointSample a, FixedPointSample b) {
        bool overflow;
        return multiply(a, b, overflow);
    }

    // Simple MAC without overflow check
    FixedPointSample macSimple(FixedPointSample a, FixedPointSample b, FixedPointSample c) {
        bool overflow;
        return mac(a, b, c, overflow);
    }

private:
    double sampleRate = 44100.0;
    float dcOffsetFilterCoeff = 0.999f;
    float dcOffsetStateL = 0.0f;
    float dcOffsetStateR = 0.0f;
};