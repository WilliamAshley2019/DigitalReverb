// PluginProcessor.cpp - PRESET LOADING FIX
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Static delay pool for all instances (properly initialized)
static std::unique_ptr<DelayMemoryPool> globalDelayPool = nullptr;
static bool globalDelayPoolInitialized = false;

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, juce::Identifier("DSP256"), createParameterLayout())
{
    dspCore = std::make_unique<FixedPointEngine>();
    effectModule = std::make_unique<ReverbHall>();
    modulationCounter = 0;
}

PluginProcessor::~PluginProcessor()
{
    if (effectModule) {
        effectModule->releaseResources();
    }
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    if (effectModule)
        return effectModule->getModuleName() + " - DSP-256";
    return "DSP-256 Effect";
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto tempModule = std::make_unique<ReverbHall>();
    auto paramDefs = tempModule->getParameterDefinitions();

    for (int i = 0; i < paramDefs.size(); ++i) {
        const auto& def = paramDefs[i];

        juce::NormalisableRange<float> range(def.minValue, def.maxValue, def.stepSize);
        if (def.isLogarithmic) {
            range.setSkewForCentre(std::sqrt(def.minValue * def.maxValue));
        }

        juce::String paramID = "param" + juce::String(i);

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(paramID, 1),
            def.name,
            range,
            def.defaultValue
        ));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (dspCore) {
        dspCore->prepare(sampleRate);
    }

    if (!globalDelayPoolInitialized) {
        globalDelayPool = std::make_unique<DelayMemoryPool>(131072);
        globalDelayPool->prepare(sampleRate);
        globalDelayPoolInitialized = true;
    }

    if (effectModule) {
        const juce::ScopedLock sl(processingLock);
        effectModule->prepare(sampleRate, samplesPerBlock);
    }

    modulationCounter = 0;
}

void PluginProcessor::releaseResources()
{
    if (effectModule) {
        const juce::ScopedLock sl(processingLock);
        effectModule->releaseResources();
    }
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    if (!dspCore || !effectModule) {
        return;
    }

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, numSamples);
    }

    const juce::ScopedLock sl(processingLock);

    // Update effect parameters from APVTS
    for (int i = 0; i < effectModule->getParameterCount(); ++i) {
        float value = parameters.getRawParameterValue("param" + juce::String(i))->load();
        effectModule->setParameter(i, value);
    }

    if (globalDelayPool && !globalDelayPoolInitialized) {
        globalDelayPool->prepare(getSampleRate());
        globalDelayPoolInitialized = true;
    }

    // Process audio samples
    for (int sample = 0; sample < numSamples; ++sample) {
        float leftFloat = buffer.getSample(0, sample);
        float rightFloat = (totalNumInputChannels > 1) ?
            buffer.getSample(1, sample) : leftFloat;

        FixedPointSample left = dspCore->floatToQ12(leftFloat);
        FixedPointSample right = dspCore->floatToQ12(rightFloat);

        if (globalDelayPool) {
            effectModule->process(left, right, *globalDelayPool, *dspCore);
        }
        else {
            static DelayMemoryPool tempPool(1024);
            static bool tempPoolPrepared = false;
            if (!tempPoolPrepared) {
                tempPool.prepare(getSampleRate());
                tempPoolPrepared = true;
            }
            effectModule->process(left, right, tempPool, *dspCore);
        }

        buffer.setSample(0, sample, dspCore->Q12ToFloat(left));
        if (totalNumOutputChannels > 1) {
            buffer.setSample(1, sample, dspCore->Q12ToFloat(right));
        }

        if (++modulationCounter >= MODULATION_UPDATE_RATE) {
            effectModule->updateModulation(modulationCounter);
            modulationCounter = 0;
        }
    }
}

//==============================================================================
// CRITICAL FIX #2: Preset loading properly updates parameters
void PluginProcessor::loadPreset(const EffectPreset& preset)
{
    if (!effectModule)
        return;

    const juce::ScopedLock sl(processingLock);

    // CRITICAL FIX: Use the effect module's own loadPreset method
    effectModule->loadPreset(preset);

    // Get parameter definitions to know the ranges
    auto paramDefs = effectModule->getParameterDefinitions();

    // Update APVTS parameters to match the preset (so GUI knobs move)
    for (int i = 0; i < juce::jmin(effectModule->getParameterCount(),
        (int)preset.parameterValues.size(),
        (int)paramDefs.size()); ++i) {

        // Get the actual value from the effect module (after preset was loaded)
        float actualValue = effectModule->getParameter(i);

        // Convert to APVTS normalized value
        const auto& def = paramDefs[i];
        float apvtsNormalized = (actualValue - def.minValue) / (def.maxValue - def.minValue);

        // Update APVTS parameter
        auto* param = parameters.getParameter("param" + juce::String(i));
        if (param) {
            param->setValueNotifyingHost(apvtsNormalized);
        }
    }

    // Update GUI if it exists
    if (auto* editor = dynamic_cast<PluginEditor*>(getActiveEditor())) {
        editor->repaint();
    }
}

//==============================================================================
juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

//==============================================================================
void PluginProcessor::setCurrentProgram(int index)
{
    if (effectModule) {
        auto presets = effectModule->getFactoryPresets();
        if (index >= 0 && index < static_cast<int>(presets.size())) {
            loadPreset(presets[static_cast<size_t>(index)]);
        }
    }
}

const juce::String PluginProcessor::getProgramName(int index)
{
    if (effectModule) {
        auto presets = effectModule->getFactoryPresets();
        if (index >= 0 && index < static_cast<int>(presets.size())) {
            return presets[static_cast<size_t>(index)].name;
        }
    }
    return "Program " + juce::String(index + 1);
}

void PluginProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState && xmlState->hasTagName(parameters.state.getType())) {
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));

        if (effectModule) {
            const juce::ScopedLock sl(processingLock);
            for (int i = 0; i < effectModule->getParameterCount(); ++i) {
                float value = parameters.getRawParameterValue("param" + juce::String(i))->load();
                effectModule->setParameter(i, value);
            }
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}