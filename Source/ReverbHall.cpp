// ReverbHall.cpp - Hall Reverb Implementation
#include "ReverbHall.h"
#include <cmath>

//==============================================================================
ReverbHall::ReverbHall()
{
    // Initialize with sensible defaults
    sampleRate = 44100.0;
    dcOffsetStateL = 0.0f;
    dcOffsetStateR = 0.0f;
    lpfStateL = 0.0f;
    lpfStateR = 0.0f;
    lfoPhase = 0.0f;
    modulationDepth = 0.0f;

    // Set reasonable initial values
    preDelay = 0.0f;          // 0ms
    decayTime = 0.7f;         // ~3.0 seconds
    diffusion = 0.8f;         // 80%
    damping = 0.5f;           // 50%
    earlyLevel = 0.7f;        // 70%
    size = 1.0f;              // 1.0x
    mix = 0.5f;               // 50%

    // Initialize buffers with default sizes
    initializeBuffers();

    // Update internal parameters
    updateParameters();
}

//==============================================================================
std::vector<EffectParameter> ReverbHall::getParameterDefinitions() const
{
    return {
        EffectParameter("predelay", "Pre-Delay", "PREDLY", "ms", 0.0f, 100.0f, 0.0f, 0.1f, false),
        EffectParameter("decay", "Decay Time", "DECAY", "s", 0.1f, 10.0f, 2.0f, 0.01f, true),
        EffectParameter("diffusion", "Diffusion", "DIFF", "%", 0.0f, 100.0f, 80.0f, 0.1f, false),
        EffectParameter("damping", "HF Damping", "DAMP", "%", 0.0f, 100.0f, 50.0f, 0.1f, false),
        EffectParameter("early", "Early Reflections", "EARLY", "%", 0.0f, 100.0f, 70.0f, 0.1f, false),
        EffectParameter("size", "Room Size", "SIZE", "x", 0.5f, 2.0f, 1.0f, 0.01f, false),
        EffectParameter("mix", "Dry/Wet Mix", "MIX", "%", 0.0f, 100.0f, 50.0f, 0.1f, false)
    };
}

//==============================================================================
std::vector<EffectPreset> ReverbHall::getFactoryPresets() const
{
    return {
        EffectPreset("Small Room", "Tight, intimate space", {0.0f, 0.3f, 0.6f, 0.7f, 0.8f, 0.6f, 0.4f}),
        EffectPreset("Medium Hall", "Balanced concert hall", {0.1f, 0.5f, 0.8f, 0.4f, 0.7f, 0.8f, 0.5f}),
        EffectPreset("Large Hall", "Spacious cathedral", {0.2f, 0.8f, 0.9f, 0.3f, 0.6f, 1.2f, 0.6f}),
        EffectPreset("Plate Verb", "Classic plate reverb", {0.0f, 0.4f, 0.9f, 0.6f, 0.5f, 0.7f, 0.5f}),
        EffectPreset("Gated Room", "80s drum reverb", {0.0f, 0.2f, 0.7f, 0.8f, 0.9f, 0.6f, 0.3f}),
        EffectPreset("Ambient", "Ethereal, long decay", {0.3f, 0.9f, 0.7f, 0.2f, 0.4f, 1.5f, 0.7f}),
        EffectPreset("Vocal Chamber", "Optimized for vocals", {0.15f, 0.45f, 0.75f, 0.55f, 0.8f, 0.8f, 0.45f}),
        EffectPreset("Reverse Tail", "Reverse reverb effect", {0.25f, 0.6f, 0.5f, 0.4f, 0.3f, 1.0f, 0.6f})
    };
}

void ReverbHall::loadPreset(const EffectPreset& preset)
{
    if (preset.parameterValues.size() >= 7) {
        preDelay = juce::jlimit(0.0f, 1.0f, preset.parameterValues[0]);
        decayTime = juce::jlimit(0.0f, 1.0f, preset.parameterValues[1]);
        diffusion = juce::jlimit(0.0f, 1.0f, preset.parameterValues[2]);
        damping = juce::jlimit(0.0f, 1.0f, preset.parameterValues[3]);
        earlyLevel = juce::jlimit(0.0f, 1.0f, preset.parameterValues[4]);
        size = juce::jlimit(0.0f, 1.0f, preset.parameterValues[5]);
        mix = juce::jlimit(0.0f, 1.0f, preset.parameterValues[6]);
        updateParameters();
    }
}

EffectPreset ReverbHall::getCurrentPreset() const
{
    return EffectPreset("Current Settings", "Current reverb parameters",
        { preDelay, decayTime, diffusion, damping, earlyLevel, size, mix });
}

void ReverbHall::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    blockSize = samplesPerBlock;
    initializeBuffers();
    updateParameters();
    reset();
}

void ReverbHall::reset()
{
    for (auto& comb : combFilters) {
        std::fill(comb.buffer.begin(), comb.buffer.end(), 0);
        comb.writeIndex = 0;
    }
    for (auto& early : earlyReflections) {
        std::fill(early.buffer.begin(), early.buffer.end(), 0);
        early.writeIndex = 0;
    }
    for (auto& ap : allpassFilters) {
        std::fill(ap.buffer.begin(), ap.buffer.end(), 0);
        ap.writeIndex = 0;
    }
    std::fill(preDelayBuffer.begin(), preDelayBuffer.end(), 0.0f);
    preDelayWriteIndex = 0;

    dampingStateL = 0;
    dampingStateR = 0;
    lpfStateL = 0.0f;
    lpfStateR = 0.0f;
    dcOffsetStateL = 0.0f;
    dcOffsetStateR = 0.0f;
    lfoPhase = 0.0f;
    currentTailLevel = 0.0f;
}

void ReverbHall::releaseResources() {}

void ReverbHall::setParameter(int parameterIndex, float value)
{
    value = juce::jlimit(0.0f, 1.0f, value);
    switch (parameterIndex) {
    case 0: preDelay = value; break;
    case 1: decayTime = value; break;
    case 2: diffusion = value; break;
    case 3: damping = value; break;
    case 4: earlyLevel = value; break;
    case 5: size = value; break;
    case 6: mix = value; break;
    }
    updateParameters();
}

float ReverbHall::getParameter(int parameterIndex) const
{
    switch (parameterIndex) {
    case 0: return preDelay;
    case 1: return decayTime;
    case 2: return diffusion;
    case 3: return damping;
    case 4: return earlyLevel;
    case 5: return size;
    case 6: return mix;
    default: return 0.0f;
    }
}

juce::String ReverbHall::getParameterDisplay(int parameterIndex) const
{
    switch (parameterIndex) {
    case 0: return juce::String(preDelay * 100.0f, 0) + " ms";
    case 1: {
        float rt60 = 0.1f * std::pow(10.0f, decayTime * 2.0f);
        return juce::String(rt60, 1) + " s";
    }
    case 2: return juce::String(diffusion * 100.0f, 0) + "%";
    case 3: return juce::String(damping * 100.0f, 0) + "%";
    case 4: return juce::String(earlyLevel * 100.0f, 0) + "%";
    case 5: return juce::String(0.5f + size * 1.5f, 2);
    case 6: return juce::String(mix * 100.0f, 0) + "%";
    default: return "";
    }
}

void ReverbHall::updateParameters()
{
    validateParameters();

    float preDelayMs = preDelay * 100.0f;
    preDelayReadOffset = static_cast<int>(preDelayMs * sampleRate / 1000.0);
    if (preDelayReadOffset < 1) preDelayReadOffset = 1;

    float rt60 = 0.1f * std::pow(10.0f, decayTime * 2.0f);
    float avgDelay = 2165.0f * size * (sampleRate / 44100.0);
    if (avgDelay < 10.0f) avgDelay = 10.0f;

    float feedbackGain = std::pow(10.0f, -3.0f * rt60 / avgDelay);
    feedbackGain = juce::jlimit(0.0f, 0.95f, feedbackGain);
    estimatedRT60 = rt60;

    for (auto& comb : combFilters) {
        comb.feedbackGain = floatToFixed(feedbackGain);
    }

    float alpha = 1.0f - damping * 0.99f;
    dampingAlpha = floatToFixed(alpha);

    for (int i = 0; i < 8; ++i) {
        float gain = earlyLevel * (0.9f - i * 0.1f);
        gain = juce::jlimit(0.0f, 1.0f, gain);
        earlyReflections[i].gain = floatToFixed(gain);
    }

    float apCoeff = diffusion * 0.7f;
    apCoeff = juce::jlimit(0.1f, 0.9f, apCoeff);
    for (auto& ap : allpassFilters) {
        ap.coeff = floatToFixed(apCoeff);
    }
}

//==============================================================================
void ReverbHall::process(FixedPointSample& left, FixedPointSample& right,
    DelayMemoryPool& delayPool, FixedPointEngine& dspCore)
{
    juce::ignoreUnused(delayPool);

    // Convert to float using new Q12 naming
    float inL = dspCore.Q12ToFloat(left);
    float inR = dspCore.Q12ToFloat(right);

    float monoIn = (inL + inR) * 0.5f;

    // DC Blocking with new Q12 naming
    FixedPointSample monoInputFP = dspCore.floatToQ12(monoIn);
    monoInputFP = dspCore.dcBlock(monoInputFP, dcOffsetStateL);
    float monoFiltered = dspCore.Q12ToFloat(monoInputFP);

    float delayedInput = monoFiltered;
    if (!preDelayBuffer.empty() && preDelayReadOffset < preDelayBuffer.size()) {
        preDelayBuffer[preDelayWriteIndex] = monoFiltered;
        int readIdx = (preDelayWriteIndex - preDelayReadOffset + (int)preDelayBuffer.size()) % (int)preDelayBuffer.size();
        delayedInput = preDelayBuffer[readIdx];
        preDelayWriteIndex = (preDelayWriteIndex + 1) % (int)preDelayBuffer.size();
    }

    // Early reflections
    float earlySum = 0.0f;
    int activeEarly = 0;

    for (int i = 0; i < 8; ++i) {
        auto& early = earlyReflections[i];
        if (early.buffer.empty() || early.delayLength < 1) continue;

        float delayed = dspCore.Q12ToFloat(FixedPointSample{ early.buffer[(early.writeIndex - early.delayLength + (int)early.buffer.size()) % (int)early.buffer.size()] });
        float gain = dspCore.Q12ToFloat(FixedPointSample{ early.gain });
        float output = delayedInput + delayed * gain;

        early.buffer[early.writeIndex] = dspCore.floatToQ12(output).value;
        early.writeIndex = (early.writeIndex + 1) % (int)early.buffer.size();

        earlySum += output;
        activeEarly++;
    }

    float earlyOut = (activeEarly > 0) ? (earlySum / activeEarly) : delayedInput;

    // Comb filters
    float combSum = 0.0f;
    int activeCombs = 0;

    for (int i = 0; i < 4; ++i) {
        auto& comb = combFilters[i];
        if (comb.buffer.empty() || comb.delayLength < 1) continue;

        float delayed = dspCore.Q12ToFloat(FixedPointSample{ comb.buffer[(comb.writeIndex - comb.delayLength + (int)comb.buffer.size()) % (int)comb.buffer.size()] });
        float feedback = dspCore.Q12ToFloat(FixedPointSample{ comb.feedbackGain });
        float output = earlyOut + delayed * feedback;

        comb.buffer[comb.writeIndex] = dspCore.floatToQ12(output).value;
        comb.writeIndex = (comb.writeIndex + 1) % (int)comb.buffer.size();

        combSum += output;
        activeCombs++;
    }

    float combOut = (activeCombs > 0) ? (combSum / activeCombs) : earlyOut;

    // Allpass filters
    float apOut = combOut;
    for (int i = 0; i < 2; ++i) {
        auto& ap = allpassFilters[i];
        if (ap.buffer.empty() || ap.delayLength < 1) continue;

        float delayed = dspCore.Q12ToFloat(FixedPointSample{ ap.buffer[(ap.writeIndex - ap.delayLength + (int)ap.buffer.size()) % (int)ap.buffer.size()] });
        float coeff = dspCore.Q12ToFloat(FixedPointSample{ ap.coeff });
        float output = delayed - coeff * apOut;
        float writeValue = apOut + coeff * delayed;

        ap.buffer[ap.writeIndex] = dspCore.floatToQ12(writeValue).value;
        ap.writeIndex = (ap.writeIndex + 1) % (int)ap.buffer.size();
        apOut = output;
    }

    float alpha = dspCore.Q12ToFloat(FixedPointSample{ dampingAlpha });
    lpfStateL = lpfStateL * alpha + apOut * (1.0f - alpha);
    lpfStateR = lpfStateR * alpha + apOut * (1.0f - alpha);

    float wetL = lpfStateL;
    float wetR = lpfStateR * 0.9f;
    float outL = inL * (1.0f - mix) + wetL * mix;
    float outR = inR * (1.0f - mix) + wetR * mix;

    left = dspCore.floatToQ12(outL);
    right = dspCore.floatToQ12(outR);

    currentTailLevel = currentTailLevel * 0.999f + std::abs(apOut) * 0.001f;
}

float ReverbHall::processCombFilter(CombFilter& comb, float input, FixedPointEngine& dsp)
{
    if (comb.buffer.empty() || comb.delayLength < 1) return input;
    float delayed = dsp.Q12ToFloat(FixedPointSample{ comb.buffer[(comb.writeIndex - comb.delayLength + (int)comb.buffer.size()) % (int)comb.buffer.size()] });
    float feedback = dsp.Q12ToFloat(FixedPointSample{ comb.feedbackGain });
    float output = input + delayed * feedback;
    comb.buffer[comb.writeIndex] = dsp.floatToQ12(output).value;
    comb.writeIndex = (comb.writeIndex + 1) % (int)comb.buffer.size();
    return output;
}

float ReverbHall::processAllpassFilter(AllpassFilter& ap, float input, FixedPointEngine& dsp)
{
    if (ap.buffer.empty() || ap.delayLength < 1) return input;
    float delayed = dsp.Q12ToFloat(FixedPointSample{ ap.buffer[(ap.writeIndex - ap.delayLength + (int)ap.buffer.size()) % (int)ap.buffer.size()] });
    float coeff = dsp.Q12ToFloat(FixedPointSample{ ap.coeff });
    float output = delayed - coeff * input;
    float writeValue = input + coeff * delayed;
    ap.buffer[ap.writeIndex] = dsp.floatToQ12(writeValue).value;
    ap.writeIndex = (ap.writeIndex + 1) % (int)ap.buffer.size();
    return output;
}

void ReverbHall::updateModulation(int blockCounter)
{
    juce::ignoreUnused(blockCounter);
    lfoPhase += 0.05f;
    if (lfoPhase > 6.28318530718f) lfoPhase -= 6.28318530718f;
}

juce::String ReverbHall::getRealtimeDisplayInfo() const
{
    float tailDB = juce::Decibels::gainToDecibels(currentTailLevel + 1e-6f);
    return juce::String::formatted("RT60: %.1fs  Tail: %.1f dB", estimatedRT60, tailDB);
}

void ReverbHall::initializeBuffers()
{
    const int baseCombDelays[] = { 1687, 1923, 2287, 2763 };
    const int baseEarlyDelays[] = { 142, 107, 379, 277, 672, 908, 445, 500 };
    const int baseAllpassDelays[] = { 389, 127 };
    float sampleRateScale = static_cast<float>(sampleRate) / 44100.0f;


    for (int i = 0; i < 4; ++i) {
        combFilters[i].delayLength = static_cast<int>(baseCombDelays[i] * sampleRateScale * size);
        if (combFilters[i].delayLength < 10) combFilters[i].delayLength = 10;
        combFilters[i].buffer.resize(combFilters[i].delayLength + 1, 0);
        combFilters[i].writeIndex = 0;
    }
    for (int i = 0; i < 8; ++i) {
        earlyReflections[i].delayLength = static_cast<int>(baseEarlyDelays[i] * sampleRateScale * size);
        if (earlyReflections[i].delayLength < 10) earlyReflections[i].delayLength = 10;
        earlyReflections[i].buffer.resize(earlyReflections[i].delayLength + 1, 0);
        earlyReflections[i].writeIndex = 0;
        earlyReflections[i].gain = floatToFixed(earlyLevel * (0.9f - i * 0.1f));
    }
    for (int i = 0; i < 2; ++i) {
        allpassFilters[i].delayLength = static_cast<int>(baseAllpassDelays[i] * sampleRateScale * size);
        if (allpassFilters[i].delayLength < 10) allpassFilters[i].delayLength = 10;
        allpassFilters[i].buffer.resize(allpassFilters[i].delayLength + 1, 0);
        allpassFilters[i].writeIndex = 0;
        allpassFilters[i].coeff = floatToFixed(diffusion * 0.7f);
    }
    int maxPreDelaySamples = static_cast<int>(sampleRate * 0.2);
    if (maxPreDelaySamples < 10) maxPreDelaySamples = 10;
    preDelayBuffer.resize(maxPreDelaySamples, 0.0f);
    preDelayWriteIndex = 0;
}

void ReverbHall::validateParameters()
{
    preDelay = juce::jlimit(0.0f, 1.0f, preDelay);
    decayTime = juce::jlimit(0.0f, 1.0f, decayTime);
    diffusion = juce::jlimit(0.0f, 1.0f, diffusion);
    damping = juce::jlimit(0.0f, 1.0f, damping);
    earlyLevel = juce::jlimit(0.0f, 1.0f, earlyLevel);
    size = juce::jlimit(0.0f, 1.0f, size);
    mix = juce::jlimit(0.0f, 1.0f, mix);
    if (size < 0.1f) size = 0.5f;
}

int32_t ReverbHall::floatToFixed(float f)
{
    // Updated to use the correct Q12 constant
    float scaled = f * static_cast<float>(FixedPointSample::Q12_ONE);
    FixedPointSample temp;
    temp.value = static_cast<int32_t>(scaled);
    temp.saturate();
    return temp.value;
}

float ReverbHall::fixedToFloat(int32_t fixed)
{
    // Updated to use the correct Q12 constant
    return static_cast<float>(fixed) / static_cast<float>(FixedPointSample::Q12_ONE);
}