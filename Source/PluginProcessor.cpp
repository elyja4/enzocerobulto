#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout VibratoProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // 1 S : velocidad (0.25x .. 4x de la velocidad propia de cada vibrato)
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "speed", 1 }, "S - Speed",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (twofar::speedMultFromParam (v), 2) + "x"; })
            .withValueFromStringFunction ([] (const String& t)
            {
                const float mult = jlimit (0.25f, 4.0f, t.getFloatValue());
                return jlimit (0.0f, 1.0f, 0.5f + std::log2 (mult) / 4.0f);
            })));

    // 2 P : profundidad (0x .. 2x)
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "depth", 1 }, "P - Profundidad",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (twofar::depthMultFromParam (v) * 100.0f)) + " %"; })
            .withValueFromStringFunction ([] (const String& t) { return jlimit (0.0f, 1.0f, t.getFloatValue() / 200.0f); })));

    // 3 D : dry / wet
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "D - Dry/Wet",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " % wet"; })
            .withValueFromStringFunction ([] (const String& t) { return jlimit (0.0f, 1.0f, t.getFloatValue() / 100.0f); })));

    // 4 V : tipo de vibrato (5 posiciones)
    StringArray names;
    for (int i = 0; i < twofar::numVibratoTypes; ++i)
        names.add (twofar::vibratoType (i).name);

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "type", 1 }, "V - Vibrato", names, 0));

    return { params.begin(), params.end() };
}

VibratoProcessor::VibratoProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    speedParam = apvts.getRawParameterValue ("speed");
    depthParam = apvts.getRawParameterValue ("depth");
    mixParam   = apvts.getRawParameterValue ("mix");
    typeParam  = apvts.getRawParameterValue ("type");
}

void VibratoProcessor::prepareToPlay (double sampleRate, int)
{
    for (int i = 0; i < 2; ++i)
        channels[i].prepare (sampleRate, i);

    mixSmooth.reset (sampleRate, 0.03);
    mixSmooth.setCurrentAndTargetValue (mixParam->load());

    setLatencySamples (twofar::baseDelaySamples);
}

bool VibratoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void VibratoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    const int   type  = juce::jlimit (0, twofar::numVibratoTypes - 1, (int) std::lround (typeParam->load()));
    const float speed = twofar::speedMultFromParam (speedParam->load());
    const float depth = twofar::depthMultFromParam (depthParam->load());
    mixSmooth.setTargetValue (mixParam->load());

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    for (int ch = 0; ch < numCh; ++ch)
        channels[ch].setTarget (type, speed, depth);

    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float mix = mixSmooth.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            const float out = channels[ch].process (data[i], mix);
            data[i] = out;
            peak = juce::jmax (peak, std::abs (out));
        }
    }

    outputPeak.store (peak);
}

juce::AudioProcessorEditor* VibratoProcessor::createEditor()
{
    return new VibratoEditor (*this);
}

void VibratoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VibratoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibratoProcessor();
}
