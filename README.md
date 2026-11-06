# 2far @enzocerobulto vibratos (VST3 para Windows / FL Studio)

Efecto de vibrato con **5 tipos**. Interfaz con tus 4 fotos, la cara de Cristo tatuada
en una esquina y SPDV en la otra.

## Las 4 perillas

| Perilla | Qué hace |
|---------|----------|
| **1 S** | Speed: velocidad del vibrato, de 0.25x a 4x (centro = velocidad normal del tipo) |
| **2 P** | Profundidad: de 0 % a 200 % (centro = 100 %) |
| **3 D** | Dry/Wet: mezcla entre señal original y vibrato |
| **4 V** | Tipo de vibrato (5 posiciones). Lleva la cara de Cristo |

El tipo también se puede elegir con los 5 botones debajo de las fotos.

## Los 5 vibratos

| Tipo | Carácter | Desafine aprox. (a S y P centrados) |
|------|----------|-------------------------------------|
| LIGERO | fino, como un vibrato de voz, 5 Hz | +/- 25 cents |
| PESADO | marcado, 3.8 Hz | +/- 108 cents (casi un semitono) |
| RAPIDO | tembloroso, 9.5 Hz | +/- 58 cents |
| CINTA | cinta gastada: wow lento + flutter + deriva aleatoria | +/- 52 cents |
| MAREO | ola lenta y profunda, desfase estéreo entre L y R | +/- 95 cents |

Esos valores se midieron con un tono de 1 kHz en las pruebas del código (`tests/dsp_test.cpp`).

## Compilar (Windows)

Igual que el bitcrusher: sube esta carpeta a un repositorio de GitHub y deja que Actions
la compile, o ejecuta `build_windows.bat` si tienes Visual Studio 2022, CMake y Git.

El plugin queda en `build/TwoFarVibratos_artefacts/Release/VST3/2far enzocerobulto vibratos.vst3`.

## Instalar en FL Studio
1. Copia la carpeta **`2far enzocerobulto vibratos.vst3`** completa a
   `C:\Program Files\Common Files\VST3`.
2. En FL Studio: **Options -> Manage plugins -> Find installed plugins**.
3. Úsalo desde el Mixer: slot de efecto vacío -> More plugins -> 2far enzocerobulto vibratos.

## Pruebas del DSP

```
g++ -std=c++17 tests/dsp_test.cpp -o dsp_test && ./dsp_test
```

## Estructura
- `Source/VibratoDSP.h`: el efecto (sin dependencias).
- `Source/PluginProcessor.*`: parámetros y audio.
- `Source/PluginEditor.*`: toda la interfaz.
- `Assets/`: las 4 fotos, el tatuaje y la cara de la perilla (se incrustan en el plugin).
