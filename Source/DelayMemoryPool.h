// DelayMemoryPool.h - Fixed for JUCE 8.0.12
#pragma once

#include <JuceHeader.h>
#include <vector>

class DelayMemoryPool {
public:
    // Ensure size is a power of two for the bitwise mask to work safely
    DelayMemoryPool(size_t requestedSize = 131072)
        : buffer(juce::nextPowerOfTwo((int)requestedSize), 0),
        mask(buffer.size() - 1),
        writePtr(0),
        sampleRate(44100.0)
    {
        juce::Random rng;
        for (auto& sample : buffer) {
            sample = rng.nextInt(juce::Range<int32_t>(-100, 100));
        }
    }

    void prepare(double sr) {
        sampleRate = sr;
    }

    int32_t readContended(int offset, int effectID, bool& contention) {
        // Removed 'static' to prevent cross-instance interference
        int busArbiter = (int)writePtr;
        contention = false;

        if ((busArbiter % 4) != effectID % 4) {
            contention = true;
            offset = juce::jmax(1, offset - 1);
        }

        // Masking is now safe because size is forced to power of two
        int readPos = static_cast<int>((static_cast<int64_t>(writePtr) - offset) & mask);
        return buffer[readPos];
    }

    void write(int32_t sample, int effectID) {
        juce::ignoreUnused(effectID);
        buffer[writePtr] = sample;
        writePtr = (writePtr + 1) & mask;

        if (juce::Random::getSystemRandom().nextInt(10000) < 1) {
            buffer[writePtr] ^= 0x01;
        }
    }

    void clear() {
        std::fill(buffer.begin(), buffer.end(), 0);
        writePtr = 0;
    }

    size_t getSize() const { return buffer.size(); }

private:
    std::vector<int32_t> buffer;
    size_t writePtr;
    size_t mask;
    double sampleRate;
};