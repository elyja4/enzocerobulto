#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "VibratoDSP.h"

class VibratoProcessor : public juce::AudioProcessor
{
public:
    VibratoProcessor();
    ~VibratoProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "2far enzocerobulto vibratos"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Nivel de salida del último bloque (la interfaz lo usa para encender las luces).
    std::atomic<float> outputPeak { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* speedParam = nullptr;   // S
    std::atomic<float>* depthParam = nullptr;   // P
    std::atomic<float>* mixParam   = nullptr;   // D
    std::atomic<float>* typeParam  = nullptr;   // V

    twofar::VibratoChannel channels[2];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoProcessor)
};
