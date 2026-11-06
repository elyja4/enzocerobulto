#pragma once
#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"

// Perillas de metal moleteado. La perilla con ID "christ" lleva la cara de Cristo como tapa
// y funciona como selector de 5 posiciones (los 5 vibratos).
class TattooLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TattooLookAndFeel();

    void setChristImage (const juce::Image& img) { christ = img; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

private:
    juce::Image christ;
};

class VibratoEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit VibratoEditor (VibratoProcessor&);
    ~VibratoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    int  currentType() const;

    void drawBackground (juce::Graphics&);
    void drawHeader (juce::Graphics&);
    void drawPhotos (juce::Graphics&);
    void drawStrip (juce::Graphics&);
    void drawKnobLabels (juce::Graphics&);

    void setupSlider (juce::Slider&);

    VibratoProcessor& proc;

    // OJO con el orden: el look-and-feel debe vivir más que los sliders,
    // y los attachments deben morir antes que los sliders.
    TattooLookAndFeel laf;
    juce::Slider sSlider, pSlider, dSlider, vSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt, pAtt, dAtt, vAtt;

    std::array<juce::Image, 4> photos;
    juce::Image tattooImg, christKnobImg;

    std::array<juce::Rectangle<float>, 4> photoRects;     // zona interior de cada foto
    std::array<juce::Rectangle<float>, 5> chipRects;      // botones de tipo de vibrato
    juce::Rectangle<float> scopeRect;
    std::array<juce::Point<float>, 220> specks;

    float uiTime  = 0.0f;
    float uiPhase = 0.0f;    // fase del "vibrato visual" que mueve las fotos
    float level   = 0.0f;    // nivel de audio suavizado (0..1)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoEditor)
};
