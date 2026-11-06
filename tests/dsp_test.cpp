#include "../Source/VibratoDSP.h"
#include <cstdio>
#include <vector>
#include <cmath>

static int fails = 0;
#define CHECK(cond, ...) do { if (!(cond)) { std::printf("FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); ++fails; } \
                              else { std::printf("ok:   "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static const float kPi = 3.14159265f;
static const double kSr = 48000.0;

// Mide la frecuencia instantánea de una senoidal por cruces por cero (ascendentes, interpolados).
static std::vector<float> instFreq (const std::vector<float>& y)
{
    std::vector<float> f;
    double last = -1.0;
    for (size_t i = 1; i < y.size(); ++i)
        if (y[i - 1] < 0.0f && y[i] >= 0.0f)
        {
            double t = (double) (i - 1) + (double) (-y[i - 1]) / (double) (y[i] - y[i - 1]);
            if (last >= 0.0) f.push_back ((float) (kSr / (t - last)));
            last = t;
        }
    return f;
}

static std::vector<float> run (int type, float speed, float depth, float mix, const std::vector<float>& in)
{
    twofar::VibratoChannel ch; ch.prepare (kSr, 0);
    ch.setTarget (type, speed, depth);
    std::vector<float> out (in.size());
    for (size_t i = 0; i < in.size(); ++i) out[i] = ch.process (in[i], mix);
    return out;
}

static std::vector<float> sine (float hz, float amp, int n)
{
    std::vector<float> v ((size_t) n);
    for (int i = 0; i < n; ++i) v[(size_t) i] = amp * std::sin (2.0f * kPi * hz * (float) i / (float) kSr);
    return v;
}

int main()
{
    // 1) mix = 0 => señal seca, retrasada exactamente baseDelaySamples
    {
        auto in = sine (440.f, 0.7f, 9600);
        auto out = run (1, 1.f, 1.f, 0.f, in);
        float err = 0.f;
        for (size_t i = twofar::baseDelaySamples; i < in.size(); ++i)
            err = std::fmax (err, std::fabs (out[i] - in[i - twofar::baseDelaySamples]));
        CHECK (err < 1e-6f, "mix 0 deja pasar la señal seca (error %.2e)", err);
    }

    // 2) profundidad 0 => la salida mojada es la entrada retrasada
    {
        auto in = sine (1000.f, 0.7f, 9600);
        auto out = run (1, 1.f, 0.f, 1.f, in);
        float err = 0.f;
        for (size_t i = 100; i < in.size(); ++i)
            err = std::fmax (err, std::fabs (out[i] - in[i - twofar::baseDelaySamples]));
        CHECK (err < 1e-4f, "profundidad 0 no altera el tono (error %.2e)", err);
    }

    // 3) la desviación de tono coincide con la teoría: 2*pi*f*profundidad
    //    se mide con un tono de 1 kHz
    struct Row { const char* name; float expectCents; };
    float measured[twofar::numVibratoTypes] = {};
    for (int t = 0; t < twofar::numVibratoTypes; ++t)
    {
        auto in = sine (1000.f, 0.8f, 48000 * 6);
        auto out = run (t, 1.f, 1.f, 1.f, in);
        auto f = instFreq (out);
        float fmax = 0.f, fmin = 1e9f;
        for (size_t i = 20; i < f.size(); ++i) { fmax = std::fmax (fmax, f[i]); fmin = std::fmin (fmin, f[i]); }
        const float cents = 0.5f * 1200.f * std::log2 (fmax / fmin);
        measured[t] = cents;
        std::printf("      %-7s desviacion de tono +/- %.1f cents\n", twofar::vibratoType (t).name, cents);
    }
    // Teoría (senoidal pura): LIGERO ~24, PESADO ~105, RAPIDO ~56 cents
    CHECK (std::fabs (measured[0] - 24.f)  < 5.f,  "LIGERO ~ +/-24 cents (medido %.1f)", measured[0]);
    CHECK (std::fabs (measured[1] - 105.f) < 15.f, "PESADO ~ +/-105 cents (medido %.1f)", measured[1]);
    CHECK (std::fabs (measured[2] - 56.f)  < 10.f, "RAPIDO ~ +/-56 cents (medido %.1f)", measured[2]);
    CHECK (measured[1] > 3.f * measured[0], "PESADO es claramente más profundo que LIGERO");
    CHECK (measured[3] > 30.f && measured[3] < 150.f, "CINTA desafina como cinta gastada (medido %.1f cents)", measured[3]);
    CHECK (measured[4] > 40.f && measured[4] < 150.f, "MAREO es profundo pero controlado (medido %.1f cents)", measured[4]);

    // 4) la velocidad del vibrato: LIGERO a 5 Hz => ~5 ciclos de la frecuencia instantánea por segundo
    {
        auto in = sine (1000.f, 0.8f, 48000 * 4);
        auto out = run (0, 1.f, 1.f, 1.f, in);
        auto f = instFreq (out);
        int crossings = 0;
        for (size_t i = 1; i < f.size(); ++i)
            if ((f[i - 1] - 1000.f) < 0.f && (f[i] - 1000.f) >= 0.f) ++crossings;
        const float hz = (float) crossings / 4.0f;
        CHECK (std::fabs (hz - 5.f) < 0.6f, "LIGERO oscila a ~5 Hz (medido %.2f Hz)", hz);
    }

    // 5) la perilla S cambia la velocidad: 2x en S=0.75
    {
        CHECK (std::fabs (twofar::speedMultFromParam (0.5f) - 1.f) < 1e-6f, "S=0.5 es velocidad normal");
        CHECK (std::fabs (twofar::speedMultFromParam (0.75f) - 2.f) < 1e-5f, "S=0.75 duplica la velocidad");
        CHECK (std::fabs (twofar::depthMultFromParam (0.5f) - 1.f) < 1e-6f, "P=0.5 es profundidad normal");
        auto in = sine (1000.f, 0.8f, 48000 * 4);
        auto out = run (0, twofar::speedMultFromParam (0.75f), 1.f, 1.f, in);
        auto f = instFreq (out);
        int crossings = 0;
        for (size_t i = 1; i < f.size(); ++i)
            if ((f[i - 1] - 1000.f) < 0.f && (f[i] - 1000.f) >= 0.f) ++crossings;
        const float hz = (float) crossings / 4.0f;
        CHECK (std::fabs (hz - 10.f) < 1.0f, "S=0.75 sube LIGERO a ~10 Hz (medido %.2f Hz)", hz);
    }

    // 6) estabilidad con valores extremos: nada de NaN, infinitos ni explosiones
    {
        bool ok = true; float peak = 0.f;
        for (int t = 0; t < twofar::numVibratoTypes && ok; ++t)
        {
            auto in = sine (200.f, 0.9f, 48000 * 2);
            auto out = run (t, 4.f, 2.f, 1.f, in);
            for (float v : out) { if (! std::isfinite (v)) ok = false; peak = std::fmax (peak, std::fabs (v)); }
        }
        CHECK (ok && peak < 1.2f, "velocidad 4x y profundidad 2x son estables (pico %.3f)", peak);
    }

    // 7) cambiar de tipo en pleno audio no produce clics
    {
        twofar::VibratoChannel ch; ch.prepare (kSr, 0);
        ch.setTarget (0, 1.f, 1.f);
        float maxStep = 0.f, prev = 0.f; int n = 0;
        for (int blk = 0; blk < 500; ++blk)
        {
            if (blk % 50 == 0) ch.setTarget ((blk / 50) % twofar::numVibratoTypes, 1.f, 1.f);
            for (int i = 0; i < 128; ++i, ++n)
            {
                float y = ch.process (0.5f * std::sin (2.f * kPi * 220.f * (float) n / (float) kSr), 1.f);
                if (n > 2000) maxStep = std::fmax (maxStep, std::fabs (y - prev));
                prev = y;
            }
        }
        // sin vibrato, el salto máximo de un seno de 220 Hz a 0.5 es ~0.0144
        CHECK (maxStep < 0.08f, "cambiar de tipo no hace clics (salto máximo %.4f)", maxStep);
    }

    // 8) los dos canales usan desfase estéreo en CINTA y MAREO
    {
        twofar::VibratoChannel l, r; l.prepare (kSr, 0); r.prepare (kSr, 1);
        l.setTarget (4, 1.f, 1.f); r.setTarget (4, 1.f, 1.f);
        float diff = 0.f;
        for (int i = 0; i < 48000; ++i)
        {
            float x = 0.5f * std::sin (2.f * kPi * 330.f * (float) i / (float) kSr);
            diff = std::fmax (diff, std::fabs (l.process (x, 1.f) - r.process (x, 1.f)));
        }
        CHECK (diff > 0.05f, "MAREO suena distinto en L y R (diferencia %.3f)", diff);
    }

    std::printf (fails ? "\n%d pruebas fallaron\n" : "\nTodas las pruebas pasaron\n", fails);
    return fails ? 1 : 0;
}
