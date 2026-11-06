#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    constexpr int kWidth  = 980;
    constexpr int kHeight = 700;

    juce::Colour bloodRed()  { return juce::Colour (0xffd4122c); }
    juce::Colour boneWhite() { return juce::Colour (0xffeee8dc); }
    juce::Colour inkBlue()   { return juce::Colour (0xff141a33); }

    juce::Font boldFont (float size)  { return juce::Font (juce::FontOptions (size, juce::Font::bold)); }
    juce::Font plainFont (float size) { return juce::Font (juce::FontOptions (size, juce::Font::plain)); }
    // Impact viene con Windows; si no existe, JUCE usa otra fuente parecida.
    juce::Font impactFont (float size) { return juce::Font (juce::FontOptions ("Impact", size, juce::Font::plain)); }

    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
}

//==============================================================================
TattooLookAndFeel::TattooLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, boneWhite());
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x88000000));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x44ffffff));
    setColour (juce::Slider::textBoxHighlightColourId, bloodRed());
}

void TattooLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float pos, float startA, float endA, juce::Slider& s)
{
    using namespace juce;

    const bool isChrist = s.getComponentID() == "christ";

    auto bounds = Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startA + pos * (endA - startA);

    auto polar = [&] (float a, float r) { return Point<float> (c.x + std::sin (a) * r, c.y - std::cos (a) * r); };

    // sombra
    g.setColour (Colour (0x77000000));
    g.fillEllipse (c.x - radius * 0.86f + 3.0f, c.y - radius * 0.86f + 6.0f, radius * 1.72f, radius * 1.72f);

    // anillo exterior de acero
    const float ringR = radius * 0.86f;
    ColourGradient ring (Colour (0xfff2f2f6), c.x - ringR * 0.7f, c.y - ringR * 0.8f,
                         Colour (0xff2c2c35), c.x + ringR * 0.7f, c.y + ringR * 0.9f, true);
    g.setGradientFill (ring);
    g.fillEllipse (c.x - ringR, c.y - ringR, ringR * 2.0f, ringR * 2.0f);

    // moleteado (dientes en el borde)
    g.setColour (Colour (0x66000000));
    for (int i = 0; i < 48; ++i)
    {
        const float a = (float) i * kTwoPi / 48.0f;
        g.drawLine (polar (a, ringR).x, polar (a, ringR).y, polar (a, ringR - 6.0f).x, polar (a, ringR - 6.0f).y, 1.4f);
    }

    // cuerpo
    const float bodyR = ringR - 8.0f;
    ColourGradient body (Colour (0xff4a4a55), c.x - bodyR * 0.5f, c.y - bodyR * 0.7f,
                         Colour (0xff0e0e14), c.x + bodyR * 0.6f, c.y + bodyR * 0.9f, true);
    g.setGradientFill (body);
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // aro de acero cepillado
    g.setColour (Colour (0x14ffffff));
    for (int k = 1; k <= 5; ++k)
    {
        const float rr = bodyR * (float) k / 6.0f;
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 1.0f);
    }

    const float outerArcR = radius * 0.95f;

    if (isChrist)
    {
        // 5 posiciones: una luz roja por cada tipo de vibrato
        const int selected = roundToInt (pos * 4.0f);
        for (int i = 0; i < 5; ++i)
        {
            const auto p = polar (startA + (float) i / 4.0f * (endA - startA), outerArcR);
            const bool on = i == selected;
            if (on)
            {
                ColourGradient glow (bloodRed().withAlpha (0.9f), p.x, p.y, bloodRed().withAlpha (0.0f), p.x + 11.0f, p.y, true);
                g.setGradientFill (glow);
                g.fillEllipse (p.x - 11.0f, p.y - 11.0f, 22.0f, 22.0f);
            }
            g.setColour (on ? Colour (0xffff6a7a) : Colour (0xff3a2a2e));
            g.fillEllipse (p.x - 3.5f, p.y - 3.5f, 7.0f, 7.0f);
        }

        // corona de espinas alrededor de la tapa
        const float capR = bodyR * 0.9f;
        g.setColour (Colour (0xff160b0b));
        for (int i = 0; i < 30; ++i)
        {
            const float a = (float) i * kTwoPi / 30.0f + 0.12f * std::sin ((float) i * 2.3f);
            const float r2 = capR + 3.0f + (float) (i % 3) * 1.5f;
            const auto p1 = polar (a, capR - 1.0f);
            const auto p2 = polar (a + 0.14f, r2);
            g.drawLine (p1.x, p1.y, p2.x, p2.y, 2.0f);
        }

        // la cara, que se inclina un poco al girar la perilla
        if (christ.isValid())
        {
            g.saveState();
            Path clip;
            clip.addEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
            g.reduceClipRegion (clip);
            g.addTransform (AffineTransform::rotation ((pos - 0.5f) * 0.9f, c.x, c.y));
            g.setImageResamplingQuality (Graphics::highResamplingQuality);
            g.drawImage (christ,
                         Rectangle<float> (c.x - capR * 1.12f, c.y - capR * 1.12f, capR * 2.24f, capR * 2.24f),
                         RectanglePlacement::stretchToFit);
            g.restoreState();
        }

        g.setColour (Colour (0xff0b0b0f));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 2.0f);

        // marca roja en el borde que apunta a la posición
        Path tip;
        tip.addTriangle (0.0f, -ringR + 1.0f, -5.0f, -ringR + 11.0f, 5.0f, -ringR + 11.0f);
        g.setColour (bloodRed());
        g.fillPath (tip, AffineTransform::rotation (angle).translated (c.x, c.y));
    }
    else
    {
        // pista y arco de valor en rojo sangre
        Path track;
        track.addCentredArc (c.x, c.y, outerArcR, outerArcR, 0.0f, startA, endA, true);
        g.setColour (Colour (0x33ffffff));
        g.strokePath (track, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

        Path val;
        val.addCentredArc (c.x, c.y, outerArcR, outerArcR, 0.0f, startA, jmax (startA + 0.001f, angle), true);
        g.setColour (bloodRed().withAlpha (0.35f));
        g.strokePath (val, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (bloodRed());
        g.strokePath (val, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // tapa central
        const float capR = bodyR * 0.62f;
        ColourGradient cap (Colour (0xffd9d9e0), c.x - capR * 0.5f, c.y - capR * 0.7f,
                            Colour (0xff55555f), c.x + capR * 0.6f, c.y + capR * 0.8f, true);
        g.setGradientFill (cap);
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        g.setColour (Colour (0xff0b0b0f));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.5f);

        // aguja
        Path pointer;
        pointer.addRoundedRectangle (-2.5f, -bodyR + 2.0f, 5.0f, bodyR * 0.55f, 2.0f);
        g.setColour (boneWhite());
        g.fillPath (pointer, AffineTransform::rotation (angle).translated (c.x, c.y));
    }
}

//==============================================================================
VibratoEditor::VibratoEditor (VibratoProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    photos[0] = juce::ImageCache::getFromMemory (BinaryData::photo1_jpg, BinaryData::photo1_jpgSize);
    photos[1] = juce::ImageCache::getFromMemory (BinaryData::photo2_jpg, BinaryData::photo2_jpgSize);
    photos[2] = juce::ImageCache::getFromMemory (BinaryData::photo3_jpg, BinaryData::photo3_jpgSize);
    photos[3] = juce::ImageCache::getFromMemory (BinaryData::photo4_jpg, BinaryData::photo4_jpgSize);
    tattooImg     = juce::ImageCache::getFromMemory (BinaryData::christ_tattoo_png, BinaryData::christ_tattoo_pngSize);
    christKnobImg = juce::ImageCache::getFromMemory (BinaryData::christ_knob_png,   BinaryData::christ_knob_pngSize);

    laf.setChristImage (christKnobImg);

    vSlider.setComponentID ("christ");
    for (auto* s : { &sSlider, &pSlider, &dSlider, &vSlider })
        setupSlider (*s);

    sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "speed", sSlider);
    pAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "depth", pSlider);
    dAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mix",   dSlider);
    vAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "type",  vSlider);

    juce::Random rng (42);
    for (auto& sp : specks)
        sp = { rng.nextFloat() * (float) kWidth, rng.nextFloat() * (float) kHeight };

    setOpaque (true);
    setResizable (false, false);
    setSize (kWidth, kHeight);
    startTimerHz (30);
}

VibratoEditor::~VibratoEditor()
{
    stopTimer();
    for (auto* s : { &sSlider, &pSlider, &dSlider, &vSlider })
        s->setLookAndFeel (nullptr);
}

void VibratoEditor::setupSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 120, 22);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                           juce::MathConstants<float>::pi * 2.8f, true);
    s.setLookAndFeel (&laf);
    addAndMakeVisible (s);
}

int VibratoEditor::currentType() const
{
    return juce::jlimit (0, twofar::numVibratoTypes - 1, juce::roundToInt ((float) vSlider.getValue()));
}

void VibratoEditor::resized()
{
    // 4 fotos en fila (zona interior; el marco se dibuja alrededor)
    for (int i = 0; i < 4; ++i)
        photoRects[(size_t) i] = { 47.0f + 230.0f * (float) i, 164.0f, 196.0f, 262.0f };

    // 5 botones de tipo + osciloscopio
    for (int i = 0; i < 5; ++i)
        chipRects[(size_t) i] = { 40.0f + 104.0f * (float) i, 446.0f, 96.0f, 32.0f };
    scopeRect = { 570.0f, 446.0f, 370.0f, 32.0f };

    // 4 columnas de perillas
    for (int i = 0; i < 4; ++i)
    {
        const int cx = 40 + 225 * i + 112;
        auto& s = (i == 0) ? sSlider : (i == 1) ? pSlider : (i == 2) ? dSlider : vSlider;
        s.setBounds (cx - 75, 532, 150, 140);
    }
}

void VibratoEditor::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < 5; ++i)
        if (chipRects[(size_t) i].contains (e.position))
            vSlider.setValue ((double) i, juce::sendNotificationSync);
}

void VibratoEditor::timerCallback()
{
    const float dt = 1.0f / 30.0f;
    uiTime += dt;

    // Velocidad visual del temblor de las fotos: sigue la velocidad real del vibrato elegido,
    // acotada para que se vea bien a 30 cuadros por segundo.
    const auto& t = twofar::vibratoType (currentType());
    const float speedMult = twofar::speedMultFromParam ((float) sSlider.getValue());
    const float visRate = juce::jlimit (0.35f, 6.0f, t.rateA * speedMult * 0.8f);
    uiPhase += visRate * dt;
    uiPhase -= std::floor (uiPhase);

    const float peak = std::sqrt (juce::jlimit (0.0f, 1.0f, proc.outputPeak.load()));
    level = juce::jmax (peak, level * 0.9f);

    repaint();
}

//==============================================================================
void VibratoEditor::paint (juce::Graphics& g)
{
    drawBackground (g);
    drawPhotos (g);
    drawHeader (g);
    drawStrip (g);
    drawKnobLabels (g);
}

void VibratoEditor::drawBackground (juce::Graphics& g)
{
    using namespace juce;

    ColourGradient bg (Colour (0xff09090d), 0.0f, 0.0f,
                       Colour (0xff1a0709), (float) kWidth, (float) kHeight, false);
    g.setGradientFill (bg);
    g.fillAll();

    // luz roja de estudio que late con el audio
    const float pulse = 0.55f + 0.45f * level;
    ColourGradient glow (bloodRed().withAlpha (0.24f * pulse), (float) kWidth * 0.82f, (float) kHeight * 0.30f,
                         bloodRed().withAlpha (0.0f), (float) kWidth * 0.82f - 480.0f, (float) kHeight * 0.30f, true);
    g.setGradientFill (glow);
    g.fillAll();

    g.setColour (Colours::white.withAlpha (0.05f));
    for (const auto& sp : specks)
        g.fillRect (sp.x, sp.y, 1.5f, 1.5f);

    // línea que separa la cabecera
    g.setColour (bloodRed().withAlpha (0.55f));
    g.drawLine (150.0f, 143.0f, 790.0f, 143.0f, 1.5f);
}

void VibratoEditor::drawHeader (juce::Graphics& g)
{
    using namespace juce;

    // Cristo tatuado sobre un parche de piel (esquina superior izquierda)
    if (tattooImg.isValid())
    {
        g.saveState();
        g.addTransform (AffineTransform::rotation (-0.06f, 73.0f, 80.0f));
        g.setColour (Colour (0x66000000));
        g.fillEllipse (18.0f, 14.0f, 120.0f, 148.0f);
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (tattooImg, Rectangle<float> (14.0f, 6.0f, 118.0f, 144.0f), RectanglePlacement::stretchToFit);
        g.restoreState();
    }

    // título: las letras ondulan al ritmo del vibrato
    const float depthV = (float) pSlider.getValue();
    const String title ("2FAR");
    const auto font = impactFont (76.0f);
    g.setFont (font);

    float total = 0.0f;
    for (int i = 0; i < title.length(); ++i)
        total += font.getStringWidthFloat (title.substring (i, i + 1)) + 3.0f;

    float x = 470.0f - total * 0.5f;
    for (int i = 0; i < title.length(); ++i)
    {
        const String ch = title.substring (i, i + 1);
        const float dy = std::sin (kTwoPi * (uiPhase + 0.13f * (float) i)) * (1.0f + 7.0f * depthV);
        const float y = 80.0f + dy;

        g.setColour (bloodRed());
        g.drawSingleLineText (ch, roundToInt (x + 4.0f), roundToInt (y + 4.0f));
        g.setColour (inkBlue());
        g.drawSingleLineText (ch, roundToInt (x - 2.0f), roundToInt (y - 2.0f));
        g.setColour (boneWhite());
        g.drawSingleLineText (ch, roundToInt (x), roundToInt (y));
        x += font.getStringWidthFloat (ch) + 3.0f;
    }

    g.setFont (boldFont (25.0f));
    g.setColour (bloodRed());
    g.drawText ("@enzocerobulto", Rectangle<float> (150.0f, 98.0f, 340.0f, 34.0f), Justification::centredRight, false);
    g.setColour (boneWhite());
    g.drawText ("  VIBRATOS", Rectangle<float> (490.0f, 98.0f, 300.0f, 34.0f), Justification::centredLeft, false);

    // SPDV en la esquina superior derecha, con letras de tatuaje (contorno + sombra)
    // (useEllipses = false: si la fuente es más ancha de lo previsto, no se corta con "...")
    const auto spdv = Rectangle<float> (796.0f, 14.0f, 176.0f, 96.0f);
    g.setFont (impactFont (70.0f));
    g.setColour (bloodRed());
    g.drawText ("SPDV", spdv.translated (5.0f, 5.0f), Justification::centred, false);
    g.setColour (inkBlue());
    for (int dx = -2; dx <= 2; dx += 2)
        for (int dy = -2; dy <= 2; dy += 2)
            if (dx != 0 || dy != 0)
                g.drawText ("SPDV", spdv.translated ((float) dx, (float) dy), Justification::centred, false);
    g.setColour (boneWhite());
    g.drawText ("SPDV", spdv, Justification::centred, false);

    // rayones de tinta bajo SPDV
    g.setColour (bloodRed().withAlpha (0.8f));
    g.drawLine (812.0f, 118.0f, 958.0f, 114.0f, 2.0f);
    g.drawLine (826.0f, 124.0f, 944.0f, 121.0f, 1.2f);
}

void VibratoEditor::drawPhotos (juce::Graphics& g)
{
    using namespace juce;

    static const float tilts[4] = { -0.045f, 0.03f, -0.025f, 0.05f };
    const float depthV = (float) pSlider.getValue();     // 0..1
    const float mixV   = (float) dSlider.getValue();     // 0..1
    const float amp    = 1.2f + 12.0f * depthV;          // píxeles de temblor
    const float ghostA = 0.10f + 0.30f * mixV;

    for (int i = 0; i < 4; ++i)
    {
        const auto inner = photoRects[(size_t) i];
        const auto frame = inner.expanded (7.0f, 7.0f);
        const auto c = frame.getCentre();
        const float wob = std::sin (kTwoPi * (uiPhase + 0.11f * (float) i));

        g.saveState();
        g.addTransform (AffineTransform::rotation (tilts[i], c.x, c.y));

        // brillo rojo detrás del marco cuando suena audio
        g.setColour (bloodRed().withAlpha (0.06f + 0.40f * level));
        g.fillRect (frame.expanded (5.0f));

        g.setColour (Colour (0x99000000));
        g.fillRect (frame.translated (5.0f, 7.0f));
        g.setColour (Colour (0xffe9e2d6));
        g.fillRect (frame);

        // foto con vibrato visual: copias fantasma a izquierda y derecha
        g.saveState();
        g.reduceClipRegion (inner.toNearestInt());
        g.setImageResamplingQuality (Graphics::highResamplingQuality);

        const float sf = 1.0f + (amp * 2.4f) / inner.getWidth();
        const auto big = inner.withSizeKeepingCentre (inner.getWidth() * sf, inner.getHeight() * sf);
        const auto& img = photos[(size_t) i];
        if (img.isValid())
        {
            g.setOpacity (ghostA);
            g.drawImage (img, big.translated (wob * amp, 0.0f), RectanglePlacement::stretchToFit);
            g.drawImage (img, big.translated (-wob * amp, 0.0f), RectanglePlacement::stretchToFit);
            g.setOpacity (1.0f);
            g.drawImage (img, big.translated (wob * amp * 0.35f, 0.0f), RectanglePlacement::stretchToFit);
        }
        else
        {
            g.setColour (Colour (0xff222228));
            g.fillRect (inner);
        }
        g.restoreState();

        // cinta adhesiva
        g.setColour (Colour (0x99d8d0b8));
        g.fillRect (Rectangle<float> (c.x - 26.0f, frame.getY() - 9.0f, 52.0f, 17.0f));

        g.restoreState();
    }
}

void VibratoEditor::drawStrip (juce::Graphics& g)
{
    using namespace juce;

    const int type = currentType();

    // botones de tipo: clic para elegir, igual que la perilla 4
    for (int i = 0; i < 5; ++i)
    {
        const auto r = chipRects[(size_t) i];
        const bool on = i == type;

        if (on)
        {
            g.setColour (bloodRed().withAlpha (0.30f + 0.25f * level));
            g.fillRoundedRectangle (r.expanded (3.0f), 8.0f);
            ColourGradient fill (bloodRed().brighter (0.2f), r.getX(), r.getY(),
                                 bloodRed().darker (0.5f), r.getX(), r.getBottom(), false);
            g.setGradientFill (fill);
        }
        else
        {
            g.setColour (Colour (0xcc0c0c12));
        }
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? Colour (0xffffb0b8) : Colour (0x44ffffff));
        g.drawRoundedRectangle (r, 6.0f, on ? 1.8f : 1.0f);

        g.setColour (on ? Colours::white : Colour (0x99eee8dc));
        g.setFont (boldFont (14.0f));
        g.drawText (twofar::vibratoType (i).name, r, Justification::centred);
    }

    // osciloscopio: dibuja el LFO real del tipo elegido con la velocidad y profundidad actuales
    g.setColour (Colour (0xcc07070b));
    g.fillRoundedRectangle (scopeRect, 6.0f);
    g.setColour (Colour (0x44ffffff));
    g.drawRoundedRectangle (scopeRect, 6.0f, 1.0f);

    const auto& t = twofar::vibratoType (type);
    const float speedMult = twofar::speedMultFromParam ((float) sSlider.getValue());
    const float depthMult = twofar::depthMultFromParam ((float) pSlider.getValue());
    const float sumDepth  = t.depthA + t.depthB;
    const float rel   = juce::jlimit (0.0f, 1.0f, sumDepth * depthMult / 17.2f);
    const float frac  = std::sqrt (rel);
    const float halfH = scopeRect.getHeight() * 0.5f - 4.0f;

    g.setColour (Colour (0x22ffffff));
    g.drawLine (scopeRect.getX() + 4.0f, scopeRect.getCentreY(), scopeRect.getRight() - 4.0f, scopeRect.getCentreY(), 1.0f);

    Path wave;
    const float x0 = scopeRect.getX() + 4.0f, w = scopeRect.getWidth() - 8.0f;
    for (float px = 0.0f; px <= w; px += 2.0f)
    {
        const float tt = px / w + uiTime;           // una ventana = 1 segundo
        const float a = std::sin (kTwoPi * t.rateA * speedMult * tt);
        const float b = std::sin (kTwoPi * t.rateB * speedMult * tt);
        const float v = sumDepth > 0.0f ? (t.depthA * a + t.depthB * b) / sumDepth : 0.0f;
        const float yy = scopeRect.getCentreY() - v * halfH * frac;
        if (px == 0.0f) wave.startNewSubPath (x0 + px, yy); else wave.lineTo (x0 + px, yy);
    }
    g.setColour (bloodRed().withAlpha (0.35f));
    g.strokePath (wave, PathStrokeType (5.0f));
    g.setColour (Colour (0xffff7a88));
    g.strokePath (wave, PathStrokeType (1.8f));
}

void VibratoEditor::drawKnobLabels (juce::Graphics& g)
{
    using namespace juce;

    static const char* nums[4]  = { "1", "2", "3", "4" };
    static const char* letts[4] = { "S", "P", "D", "V" };
    static const char* subs[4]  = { "SPEED", "PROFUNDIDAD", "DRY / WET", "VIBRATO  -  TIPO" };

    for (int i = 0; i < 4; ++i)
    {
        const float cx = 40.0f + 225.0f * (float) i + 112.5f;

        g.setFont (boldFont (26.0f));
        g.setColour (bloodRed());
        g.drawText (nums[i], Rectangle<float> (cx - 80.0f, 492.0f, 74.0f, 42.0f), Justification::centredRight);

        g.setFont (impactFont (46.0f));
        g.setColour (boneWhite());
        g.drawText (letts[i], Rectangle<float> (cx + 2.0f, 488.0f, 80.0f, 48.0f), Justification::centredLeft);

        g.setFont (plainFont (12.0f));
        g.setColour (Colour (0x99eee8dc));
        g.drawText (subs[i], Rectangle<float> (cx - 100.0f, 674.0f, 200.0f, 18.0f), Justification::centred);

        if (i > 0)
        {
            g.setColour (Colour (0x22ffffff));
            g.drawLine (40.0f + 225.0f * (float) i, 496.0f, 40.0f + 225.0f * (float) i, 686.0f, 1.0f);
        }
    }
}
