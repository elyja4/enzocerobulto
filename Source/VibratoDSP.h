#pragma once
// Núcleo DSP de los vibratos. No depende de JUCE, así se puede probar solo.
//
// Un vibrato es una línea de retardo cuyo tiempo se modula con un LFO: al variar el retardo,
// la señal cambia de tono hacia arriba y hacia abajo. Aquí se lee con interpolación cúbica
// (Hermite) para que no suene áspero.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace twofar
{
    constexpr int numVibratoTypes = 5;

    // El retardo mínimo de lectura (en muestras). La señal seca se retrasa lo mismo,
    // así con profundidad 0 la mezcla seco/mojado queda perfectamente alineada.
    constexpr int baseDelaySamples = 2;

    struct VibratoType
    {
        const char* name;
        float rateA, depthA;     // LFO principal: Hz, profundidad en ms
        float rateB, depthB;     // segundo LFO (flutter, armónico...): Hz, ms
        float noiseDepth;        // deriva aleatoria suave: ms
        float noiseHz;           // qué tan rápido deriva
        float stereo;            // desfase L/R en ciclos (0 = idéntico en ambos canales)
    };

    // La desviación de tono máxima de un LFO senoidal es 2*pi*frecuencia*profundidad.
    //   LIGERO : 5.0 Hz, 0.45 ms -> ~ +/-24 cents (vibrato fino, tipo voz)
    //   PESADO : 3.8 Hz, 2.6 ms  -> ~ +/-105 cents (casi un semitono, muy marcado)
    //   RAPIDO : 9.5 Hz, 0.55 ms -> ~ +/-56 cents (tembloroso)
    //   CINTA  : wow 0.7 Hz + flutter 9.1 Hz + deriva (cinta gastada)
    //   MAREO  : 1.1 Hz y 2.2 Hz, 7 ms (ola lenta y profunda, desafinada)
    inline const VibratoType& vibratoType (int index)
    {
        static const VibratoType types[numVibratoTypes] = {
            { "LIGERO", 5.0f, 0.45f, 0.0f, 0.00f, 0.00f, 1.0f, 0.00f },
            { "PESADO", 3.8f, 2.60f, 0.0f, 0.00f, 0.00f, 1.0f, 0.00f },
            { "RAPIDO", 9.5f, 0.55f, 0.0f, 0.00f, 0.00f, 1.0f, 0.00f },
            { "CINTA",  0.7f, 3.00f, 9.1f, 0.25f, 0.30f, 1.5f, 0.10f },
            { "MAREO",  1.1f, 7.00f, 2.2f, 1.60f, 0.00f, 1.0f, 0.25f },
        };
        return types[std::min (std::max (index, 0), numVibratoTypes - 1)];
    }

    // Perilla S (0..1) -> multiplicador de velocidad 0.25x .. 4x (0.5 = velocidad normal del tipo)
    inline float speedMultFromParam (float s) noexcept { return std::exp2 (4.0f * (s - 0.5f)); }

    // Perilla P (0..1) -> multiplicador de profundidad 0x .. 2x (0.5 = profundidad normal del tipo)
    inline float depthMultFromParam (float p) noexcept { return 2.0f * p; }

    // Un canal de vibrato.
    class VibratoChannel
    {
    public:
        void prepare (double sampleRate, int channelIndex)
        {
            sr = (float) sampleRate;
            channel = channelIndex;

            const int needed = (int) (sr * 0.080f) + 16;   // 80 ms de colchón
            size = 1;
            while (size < needed) size <<= 1;
            mask = size - 1;
            buffer.assign ((size_t) size, 0.0f);

            rateCoef  = 1.0f - std::exp (-1.0f / (0.050f * sr));
            depthCoef = 1.0f - std::exp (-1.0f / (0.080f * sr));
            reset();
        }

        void reset()
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
            phaseA = phaseB = 0.0f;
            noiseLP = noiseLP2 = 0.0f;
            rng = 0x9E3779B9u + (uint32_t) channel * 7919u;
            snapToTarget = true;
        }

        // Se llama una vez por bloque.
        void setTarget (int type, float speedMult, float depthMult) noexcept
        {
            const auto& t = vibratoType (type);
            tgtRateA  = t.rateA * speedMult;
            tgtRateB  = t.rateB * speedMult;
            tgtDepthA = t.depthA * depthMult;
            tgtDepthB = t.depthB * depthMult;
            tgtNoise  = t.noiseDepth * depthMult;
            tgtStereo = t.stereo;
            noiseCoef = 1.0f - std::exp (-6.2831853f * std::max (0.05f, t.noiseHz * speedMult) / sr);

            if (snapToTarget)
            {
                curRateA = tgtRateA;   curRateB = tgtRateB;
                curDepthA = tgtDepthA; curDepthB = tgtDepthB;
                curNoise = tgtNoise;   curStereo = tgtStereo;
                snapToTarget = false;
            }
        }

        // mix: 0 = solo señal seca, 1 = solo vibrato
        float process (float x, float mix) noexcept
        {
            // Todo cambia con suavizado para que mover perillas o cambiar de tipo no haga clics.
            curRateA  += (tgtRateA  - curRateA)  * rateCoef;
            curRateB  += (tgtRateB  - curRateB)  * rateCoef;
            curDepthA += (tgtDepthA - curDepthA) * depthCoef;
            curDepthB += (tgtDepthB - curDepthB) * depthCoef;
            curNoise  += (tgtNoise  - curNoise)  * depthCoef;
            curStereo += (tgtStereo - curStereo) * depthCoef;

            phaseA += curRateA / sr;  phaseA -= std::floor (phaseA);
            phaseB += curRateB / sr;  phaseB -= std::floor (phaseB);

            // Deriva aleatoria suave: ruido blanco por dos filtros de un polo en cascada
            // (así no tiene saltos bruscos) y normalizado a ~[-1, 1].
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            const float white = (float) (rng & 0xFFFFFF) / 8388608.0f - 1.0f;
            noiseLP  += (white   - noiseLP)  * noiseCoef;
            noiseLP2 += (noiseLP - noiseLP2) * noiseCoef;
            const float pole = 1.0f - noiseCoef, q = pole * pole;
            const float variance = (1.0f / 3.0f) * std::pow (noiseCoef, 4.0f) * (1.0f + q) / std::pow (1.0f - q, 3.0f);
            const float noiseVal = std::min (1.0f, std::max (-1.0f, noiseLP2 / (2.0f * std::sqrt (variance))));

            const float chOffset = curStereo * (float) channel;
            const float twoPi = 6.2831853f;
            const float modA = std::sin (twoPi * (phaseA + chOffset));
            const float modB = std::sin (twoPi * (phaseB + chOffset));

            const float modMs    = curDepthA * modA + curDepthB * modB + curNoise * noiseVal;
            const float centreMs = curDepthA + curDepthB + curNoise;   // evita retardos negativos

            lastMod = centreMs > 1.0e-6f ? modMs / centreMs : 0.0f;

            float delay = (float) baseDelaySamples + (centreMs + modMs) * 0.001f * sr;
            delay = std::min (std::max (delay, (float) baseDelaySamples), (float) (size - 4));

            buffer[(size_t) writePos] = x;

            const float readPos = (float) writePos - delay;
            const int   i0 = (int) std::floor (readPos);
            const float f  = readPos - (float) i0;

            const float y0 = buffer[(size_t) ((i0 - 1 + size) & mask)];
            const float y1 = buffer[(size_t) ((i0     + size) & mask)];
            const float y2 = buffer[(size_t) ((i0 + 1 + size) & mask)];
            const float y3 = buffer[(size_t) ((i0 + 2 + size) & mask)];

            const float c0 = y1;
            const float c1 = 0.5f * (y2 - y0);
            const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            const float wet = ((c3 * f + c2) * f + c1) * f + c0;

            const float dry = buffer[(size_t) ((writePos - baseDelaySamples + size) & mask)];

            writePos = (writePos + 1) & mask;
            return dry + (wet - dry) * mix;
        }

        // Modulación actual normalizada (-1..1), para dibujar o depurar.
        float getModulation() const noexcept { return lastMod; }

    private:
        std::vector<float> buffer;
        int size = 0, mask = 0, writePos = 0, channel = 0;
        float sr = 44100.0f;

        float phaseA = 0.0f, phaseB = 0.0f, noiseLP = 0.0f, noiseLP2 = 0.0f, lastMod = 0.0f;
        float rateCoef = 0.0f, depthCoef = 0.0f, noiseCoef = 0.0f;
        float tgtRateA = 0, tgtRateB = 0, tgtDepthA = 0, tgtDepthB = 0, tgtNoise = 0, tgtStereo = 0;
        float curRateA = 0, curRateB = 0, curDepthA = 0, curDepthB = 0, curNoise = 0, curStereo = 0;
        uint32_t rng = 1;
        bool snapToTarget = true;
    };
}
