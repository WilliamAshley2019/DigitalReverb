// EffectBlock.cpp - Implementation
#include "EffectBlock.h"

juce::String EffectBlock::getEffectName(Type type)
{
    switch (type)
    {
    case OFF: return "Off";
    case REVERB_HALL: return "Hall Reverb";
    case REVERB_PLATE: return "Plate Reverb";
    case REVERB_ROOM: return "Room Reverb";
    case REVERB_GATED: return "Gated Reverb";
    case REVERB_REVERSE: return "Reverse Reverb";
    case DELAY_MONO: return "Mono Delay";
    case DELAY_STEREO: return "Stereo Delay";
    case DELAY_PINGPONG: return "Ping Pong Delay";
    case DELAY_TAPE: return "Tape Delay";
    case CHORUS: return "Chorus";
    case FLANGER: return "Flanger";
    case PITCH_SHIFT: return "Pitch Shift";
    case PARAMETRIC_EQ: return "Parametric EQ";
    case GRAPHIC_EQ: return "Graphic EQ";
    case PHASER: return "Phaser";
    case TREMOLO: return "Tremolo";
    case ROTARY: return "Rotary Speaker";
    case COMPRESSOR: return "Compressor";
    case LIMITER: return "Limiter";
    case NOISE_GATE: return "Noise Gate";
    case DISTORTION: return "Distortion";
    case FILTER_LPF: return "Low-Pass Filter";
    case FILTER_HPF: return "High-Pass Filter";
    default: return "Unknown";
    }
}