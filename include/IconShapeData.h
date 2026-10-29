#ifndef ICON_SHAPE_DATA_H
#define ICON_SHAPE_DATA_H

// Diese Datei wird erzeugt. Nicht von Hand bearbeiten.
// Quelle der Formen: formen.json im Ordner WaveShare-Symbole, Erzeuger: erzeuge_cpp.py
// Herkunft der Geometrie: Phosphor Icons (MIT) für zwölf Symbole, Lucide (ISC) für die Werkstatt,
// Eigenmaterial des Projektinhabers für Kurzzeitmesser und Musikspieler, eigener Entwurf für die Stummschaltung.
// Das QR-Symbol ist aus Phosphors qr-code und Grundformen des Zeichners zusammengesetzt.
// Die Lizenzvermerke stehen in docs/LIZENZEN.md.

#include "IconShapes.h"

namespace
{
    /**
     * @brief Gemeinsame Punkttabelle aller Streckenzüge, paarweise als x und y
     */
    constexpr float ICON_POINTS[] = {
        56.071f, 50.714f, 56.071f, 77.5f, 39.997f, 69.464f, 23.929f, 77.5f, 23.929f, 50.718f, 2.5f, 75.938f,
        5.62f, 71.244f, 9.275f, 67.004f, 13.421f, 63.271f, 18.016f, 60.097f, 23.014f, 57.537f, 23.014f, 57.537f,
        28.374f, 55.642f, 34.05f, 54.466f, 40.0f, 54.062f, 45.95f, 54.466f, 51.626f, 55.642f, 51.626f, 55.642f,
        56.986f, 57.537f, 61.984f, 60.097f, 66.579f, 63.271f, 70.725f, 67.004f, 74.38f, 71.244f, 74.38f, 71.244f,
        77.5f, 75.938f, 25.938f, 52.5f, 35.312f, 61.875f, 54.062f, 43.125f, 17.333f, 15.05f, 1.69f, 15.05f,
        1.295f, 14.915f, 1.128f, 14.724f, 1.04f, 14.4f, 1.04f, 3.77f, 1.04f, 3.77f, 17.255f, 3.77f,
        17.684f, 3.916f, 17.866f, 4.123f, 17.96f, 4.475f, 17.96f, 14.423f, 17.96f, 14.423f, 17.776f, 14.866f,
        17.333f, 15.05f, 1.04f, 3.77f, 1.064f, 1.473f, 1.316f, 1.096f, 1.745f, 0.95f, 6.621f, 0.99f,
        6.886f, 1.156f, 6.886f, 1.156f, 9.5f, 3.77f, 10.287f, 4.425f, 6.713f, 0.85f, 3.138f, 4.425f,
        13.863f, 15.15f, 12.929f, 15.089f, 12.012f, 14.906f, 11.126f, 14.606f, 10.288f, 14.192f, 9.51f, 13.672f,
        9.51f, 13.672f, 8.807f, 13.056f, 8.19f, 12.353f, 7.67f, 11.575f, 7.257f, 10.736f, 6.956f, 9.851f,
        6.956f, 9.851f, 6.774f, 8.933f, 6.713f, 8.0f, 6.713f, 0.85f, 8.5f, 10.0f, 3.0f, 10.0f,
        3.0f, 18.0f, 8.5f, 18.0f, 14.0f, 24.0f, 14.0f, 4.0f, 60.455f, 7.657f, 65.838f, 11.845f,
        68.955f, 15.087f, 72.825f, 20.571f, 74.812f, 24.611f, 76.806f, 31.193f, 76.806f, 31.193f, 77.421f, 35.899f,
        77.151f, 43.445f, 76.109f, 48.457f, 74.395f, 53.279f, 72.041f, 57.824f, 72.041f, 57.824f, 69.089f, 62.005f,
        65.596f, 65.746f, 61.626f, 68.976f, 57.252f, 71.635f, 52.558f, 73.674f, 52.558f, 73.674f, 47.63f, 75.055f,
        42.559f, 75.752f, 37.441f, 75.752f, 32.37f, 75.055f, 27.442f, 73.674f, 27.442f, 73.674f, 22.748f, 71.635f,
        18.374f, 68.976f, 14.404f, 65.746f, 10.911f, 62.005f, 7.959f, 57.824f, 7.959f, 57.824f, 5.605f, 53.279f,
        3.891f, 48.457f, 2.849f, 43.445f, 2.579f, 35.899f, 3.194f, 31.193f, 3.194f, 31.193f, 5.188f, 24.611f,
        7.175f, 20.571f, 11.045f, 15.087f, 14.162f, 11.845f, 19.545f, 7.657f, 56.68f, 66.26f, 55.99f, 66.84f,
        55.99f, 69.15f, 56.45f, 69.73f, 57.49f, 69.27f, 57.49f, 66.84f, 57.14f, 66.38f, 23.2f, 66.26f,
        22.51f, 66.73f, 22.51f, 69.27f, 22.97f, 69.73f, 24.01f, 69.15f, 23.66f, 66.38f, 51.14f, 63.72f,
        50.68f, 64.3f, 50.68f, 69.15f, 51.26f, 69.73f, 52.18f, 69.27f, 52.18f, 64.07f, 51.83f, 63.72f,
        28.51f, 63.03f, 27.82f, 63.61f, 27.82f, 69.27f, 28.28f, 69.73f, 29.32f, 69.15f, 29.09f, 63.26f,
        45.6f, 60.84f, 45.14f, 61.3f, 45.14f, 69.27f, 45.6f, 69.73f, 46.64f, 69.15f, 46.64f, 61.41f,
        46.18f, 60.84f, 33.94f, 59.57f, 33.36f, 60.26f, 33.36f, 69.15f, 33.94f, 69.73f, 34.86f, 69.27f,
        34.86f, 60.03f, 34.4f, 59.57f, 39.83f, 57.26f, 39.25f, 57.95f, 39.25f, 69.15f, 39.71f, 69.73f,
        40.75f, 69.27f, 40.75f, 57.84f, 40.4f, 57.37f, 14.78f, 51.49f, 15.35f, 52.41f, 25.86f, 52.41f,
        26.44f, 53.68f, 28.4f, 54.95f, 31.05f, 54.49f, 32.55f, 52.41f, 64.65f, 52.41f, 65.22f, 51.72f,
        64.76f, 50.91f, 32.55f, 50.91f, 31.28f, 48.95f, 29.67f, 48.25f, 27.24f, 48.83f, 25.74f, 50.91f,
        15.24f, 50.91f, 28.17f, 49.99f, 29.55f, 49.76f, 30.82f, 50.68f, 31.05f, 51.95f, 30.71f, 52.76f,
        29.67f, 53.45f, 28.17f, 53.22f, 27.24f, 51.83f, 27.59f, 50.56f, 36.02f, 20.55f, 34.98f, 21.01f,
        34.29f, 22.16f, 34.29f, 35.21f, 35.21f, 36.59f, 36.94f, 36.83f, 47.56f, 30.13f, 48.14f, 28.05f,
        47.1f, 26.67f, 36.59f, 20.55f, 36.13f, 22.05f, 46.64f, 28.28f, 46.29f, 29.21f, 36.36f, 35.44f,
        35.79f, 35.09f, 35.79f, 22.39f, 38.56f, 12.58f, 34.17f, 13.62f, 29.55f, 16.39f, 26.2f, 20.43f,
        25.05f, 22.86f, 24.13f, 26.67f, 24.59f, 32.67f, 26.2f, 36.48f, 27.47f, 38.33f, 31.86f, 42.14f,
        35.56f, 43.75f, 38.56f, 44.33f, 44.56f, 43.75f, 48.02f, 42.25f, 50.33f, 40.63f, 54.03f, 36.13f,
        55.18f, 33.48f, 55.87f, 30.24f, 55.41f, 24.24f, 54.14f, 21.01f, 52.29f, 18.24f, 48.02f, 14.66f,
        44.56f, 13.16f, 41.56f, 12.58f, 38.67f, 14.08f, 43.98f, 14.54f, 49.06f, 17.2f, 52.64f, 21.47f,
        54.37f, 26.67f, 54.49f, 28.86f, 53.91f, 32.44f, 51.37f, 37.4f, 47.22f, 40.98f, 44.33f, 42.25f,
        41.56f, 42.83f, 36.13f, 42.37f, 31.28f, 39.94f, 27.47f, 35.67f, 25.63f, 30.24f, 25.97f, 24.93f,
        27.36f, 21.47f, 28.98f, 19.16f, 33.25f, 15.7f, 35.79f, 14.66f, 38.56f, 14.2f, 11.31f, 4.5f,
        9.23f, 6.81f, 8.2f, 9.58f, 8.2f, 70.42f, 8.66f, 72.15f, 10.04f, 74.34f, 12.0f, 75.96f,
        15.35f, 77.0f, 64.76f, 77.0f, 66.84f, 76.54f, 69.15f, 75.15f, 71.23f, 72.38f, 71.8f, 70.42f,
        71.8f, 9.7f, 71.11f, 7.39f, 69.84f, 5.54f, 67.07f, 3.58f, 64.53f, 3.0f, 15.35f, 3.0f,
        12.93f, 3.58f, 11.77f, 6.12f, 14.89f, 4.62f, 65.11f, 4.62f, 67.3f, 5.42f, 69.03f, 6.93f,
        70.19f, 9.23f, 70.42f, 11.66f, 70.42f, 68.34f, 69.84f, 71.8f, 68.0f, 74.11f, 65.22f, 75.38f,
        14.78f, 75.38f, 12.47f, 74.46f, 10.97f, 73.07f, 9.7f, 70.3f, 9.7f, 9.7f, 10.5f, 7.62f,
        39.59f, 3.0f, 38.3f, 3.55f, 37.38f, 5.02f, 37.29f, 13.74f, 32.7f, 11.45f, 27.83f, 10.34f,
        23.98f, 10.44f, 21.87f, 10.99f, 20.4f, 11.81f, 20.58f, 12.82f, 24.35f, 13.93f, 27.83f, 15.58f,
        23.7f, 16.13f, 19.57f, 17.23f, 15.62f, 19.07f, 12.14f, 21.55f, 8.74f, 25.4f, 6.35f, 29.72f,
        4.79f, 34.58f, 4.15f, 39.17f, 4.33f, 47.07f, 5.52f, 53.86f, 8.0f, 60.11f, 11.58f, 65.34f,
        16.17f, 69.75f, 22.33f, 73.51f, 29.3f, 75.9f, 37.2f, 77.0f, 45.19f, 76.82f, 52.17f, 75.53f,
        57.49f, 73.6f, 61.25f, 71.58f, 64.1f, 69.56f, 68.97f, 64.7f, 72.64f, 58.82f, 74.93f, 52.03f,
        75.76f, 45.51f, 75.85f, 39.27f, 74.93f, 33.39f, 72.91f, 28.16f, 69.79f, 23.47f, 66.12f, 20.17f,
        62.26f, 17.97f, 57.21f, 16.31f, 52.17f, 15.58f, 55.38f, 14.02f, 59.42f, 12.82f, 59.69f, 11.91f,
        58.32f, 11.08f, 55.29f, 10.34f, 50.79f, 10.53f, 47.3f, 11.45f, 42.71f, 13.74f, 42.62f, 4.93f,
        42.16f, 4.01f, 40.41f, 3.0f, 5.43f, 46.7f, 8.0f, 48.91f, 10.12f, 50.19f, 13.6f, 51.75f,
        18.29f, 53.22f, 28.39f, 55.06f, 38.49f, 55.7f, 48.95f, 55.33f, 59.88f, 53.68f, 68.6f, 50.83f,
        71.63f, 49.18f, 74.57f, 46.7f, 73.65f, 52.76f, 71.9f, 57.81f, 69.15f, 62.59f, 65.02f, 67.27f,
        60.43f, 70.76f, 54.74f, 73.51f, 48.31f, 75.26f, 42.52f, 75.9f, 35.09f, 75.71f, 28.11f, 74.43f,
        23.24f, 72.68f, 17.18f, 69.1f, 12.69f, 64.88f, 8.92f, 59.46f, 6.53f, 53.4f, 5.43f, 47.07f,
        39.86f, 58.18f, 39.5f, 58.45f, 39.5f, 63.87f, 40.05f, 64.24f, 40.5f, 63.96f, 40.5f, 58.45f,
        40.14f, 58.18f, 48.4f, 57.9f, 47.94f, 58.36f, 47.94f, 60.84f, 48.31f, 61.21f, 49.04f, 60.93f,
        49.04f, 58.27f, 48.68f, 57.9f, 31.42f, 57.9f, 30.96f, 58.27f, 30.96f, 60.84f, 31.32f, 61.21f,
        32.06f, 60.75f, 32.06f, 58.36f, 31.78f, 58.0f, 55.93f, 56.99f, 55.65f, 57.26f, 55.65f, 60.29f,
        56.2f, 60.66f, 56.66f, 60.38f, 56.76f, 59.46f, 56.66f, 57.26f, 56.39f, 56.99f, 23.61f, 56.99f,
        23.34f, 57.26f, 23.34f, 60.38f, 23.7f, 60.66f, 24.35f, 60.29f, 24.35f, 57.26f, 24.07f, 56.99f,
        62.26f, 55.33f, 61.99f, 55.61f, 61.99f, 57.9f, 62.72f, 58.18f, 62.91f, 55.61f, 62.63f, 55.33f,
        17.37f, 55.33f, 17.0f, 55.88f, 17.09f, 58.0f, 17.37f, 58.18f, 18.01f, 57.9f, 18.01f, 55.61f,
        17.74f, 55.33f, 67.59f, 53.13f, 67.22f, 53.5f, 67.22f, 55.7f, 67.59f, 56.07f, 68.23f, 55.79f,
        68.23f, 53.4f, 67.96f, 53.13f, 12.04f, 53.13f, 11.77f, 53.4f, 11.77f, 55.79f, 12.04f, 56.07f,
        12.78f, 55.7f, 12.78f, 53.5f, 12.5f, 53.13f, 74.57f, 37.15f, 74.57f, 44.22f, 73.19f, 46.43f,
        70.8f, 48.35f, 67.22f, 50.19f, 61.81f, 52.03f, 55.93f, 53.31f, 48.58f, 54.23f, 41.24f, 54.6f,
        32.52f, 54.32f, 25.26f, 53.5f, 18.56f, 52.12f, 12.78f, 50.19f, 8.37f, 47.8f, 6.08f, 45.51f,
        5.25f, 43.03f, 5.34f, 38.07f, 6.17f, 33.57f, 7.91f, 28.98f, 10.85f, 24.48f, 14.16f, 21.36f,
        17.64f, 19.25f, 21.87f, 17.69f, 26.46f, 16.86f, 30.04f, 16.77f, 30.41f, 17.14f, 25.72f, 19.43f,
        22.69f, 22.0f, 19.66f, 26.6f, 20.03f, 27.88f, 21.04f, 27.97f, 24.35f, 26.14f, 29.12f, 24.3f,
        35.36f, 23.01f, 35.46f, 27.42f, 36.1f, 30.64f, 36.65f, 31.74f, 37.84f, 32.01f, 38.49f, 31.65f,
        39.86f, 29.72f, 43.99f, 23.11f, 44.73f, 22.92f, 49.32f, 23.84f, 53.08f, 25.03f, 59.05f, 27.97f,
        59.79f, 27.97f, 60.24f, 27.61f, 60.43f, 26.78f, 59.05f, 24.21f, 57.31f, 22.0f, 53.73f, 19.07f,
        49.59f, 17.05f, 50.7f, 16.68f, 55.65f, 17.14f, 58.78f, 17.87f, 62.91f, 19.53f, 66.21f, 21.64f,
        68.42f, 23.66f, 70.34f, 26.04f, 72.36f, 29.53f, 73.83f, 33.57f, 74.48f, 37.06f, 36.92f, 47.44f,
        36.74f, 48.08f, 38.76f, 51.94f, 39.5f, 53.22f, 40.23f, 53.5f, 43.26f, 48.08f, 43.17f, 47.53f,
        37.11f, 47.34f, 38.3f, 48.45f, 41.79f, 48.54f, 40.05f, 51.75f, 38.21f, 48.54f, 22.33f, 12.09f,
        24.81f, 11.45f, 27.56f, 11.45f, 33.16f, 12.82f, 37.38f, 15.21f, 36.74f, 18.06f, 37.66f, 18.33f,
        38.49f, 15.76f, 38.49f, 5.11f, 39.4f, 4.19f, 40.6f, 4.19f, 41.51f, 5.2f, 41.51f, 15.67f,
        41.97f, 17.69f, 42.62f, 18.42f, 43.26f, 17.97f, 42.62f, 15.21f, 46.66f, 12.92f, 52.26f, 11.45f,
        55.1f, 11.45f, 57.58f, 12.09f, 51.89f, 14.38f, 47.48f, 16.77f, 47.21f, 17.41f, 47.67f, 17.87f,
        50.88f, 18.79f, 54.55f, 21.0f, 57.12f, 23.47f, 59.14f, 26.6f, 51.43f, 23.29f, 43.72f, 21.64f,
        37.48f, 30.91f, 36.47f, 26.69f, 36.74f, 22.1f, 36.01f, 21.64f, 30.59f, 22.74f, 26.73f, 23.93f,
        20.95f, 26.69f, 22.88f, 23.47f, 25.72f, 20.81f, 29.4f, 18.7f, 32.33f, 17.87f, 32.79f, 17.41f,
        32.52f, 16.77f, 27.93f, 14.29f, 22.69f, 12.27f, 15.04f, 28.4f, 19.52f, 28.4f, 19.52f, 19.44f,
        2.5f, 24.375f, 74.375f, 24.375f, 75.938f, 24.794f, 77.081f, 25.938f, 77.5f, 27.5f, 77.5f, 71.25f,
        77.5f, 71.25f, 77.081f, 72.812f, 75.938f, 73.956f, 74.375f, 74.375f, 5.625f, 74.375f, 4.062f, 73.956f,
        4.062f, 73.956f, 2.919f, 72.812f, 2.5f, 71.25f, 2.5f, 24.375f, 2.5f, 12.727f, 9.318f, 19.545f,
        22.955f, 5.909f, 2.5f, 40.0f, 9.318f, 46.818f, 22.955f, 33.182f, 2.5f, 67.273f, 9.318f, 74.091f,
        22.955f, 60.455f, 28.204f, 47.052f, 28.649f, 43.605f, 29.575f, 40.254f, 30.962f, 37.066f, 32.783f, 34.106f,
        35.003f, 31.43f, 35.003f, 31.43f, 37.576f, 29.093f, 40.452f, 27.141f, 43.575f, 25.613f, 46.881f, 24.538f,
        50.305f, 23.939f, 50.305f, 23.939f, 53.779f, 23.828f, 57.235f, 24.205f, 60.603f, 25.065f, 63.817f, 26.389f,
        66.813f, 28.153f, 66.813f, 28.153f, 69.532f, 30.319f, 71.918f, 32.847f, 73.926f, 35.684f, 75.515f, 38.776f,
        76.654f, 42.06f, 76.654f, 42.06f, 77.32f, 45.472f, 77.5f, 48.944f, 77.19f, 52.406f, 76.396f, 55.79f,
        75.135f, 59.03f, 75.135f, 59.03f, 73.431f, 62.06f, 71.318f, 64.82f, 68.838f, 67.256f, 66.04f, 69.319f,
        62.98f, 70.968f, 62.98f, 70.968f, 59.719f, 72.171f, 56.32f, 72.904f, 52.853f, 73.151f, 23.854f, 73.151f,
        19.486f, 72.537f, 19.486f, 72.537f, 15.453f, 70.75f, 12.064f, 67.926f, 10.694f, 66.191f, 9.578f, 64.283f,
        8.185f, 60.097f, 8.185f, 60.097f, 7.992f, 55.69f, 8.353f, 53.509f, 9.958f, 49.4f, 11.171f, 47.552f,
        12.627f, 45.888f, 12.627f, 45.888f, 16.156f, 43.242f, 20.275f, 41.663f, 22.458f, 41.315f, 24.669f, 41.272f,
        29.001f, 42.101f, 15.028f, 43.917f, 13.6f, 42.006f, 11.59f, 37.691f, 10.825f, 32.993f, 10.931f, 30.61f,
        11.362f, 28.263f, 11.362f, 28.263f, 13.16f, 23.856f, 16.085f, 20.102f, 17.905f, 18.559f, 22.088f, 16.288f,
        26.73f, 15.236f, 26.73f, 15.236f, 29.116f, 15.195f, 33.791f, 16.088f, 35.993f, 17.005f, 39.921f, 19.694f,
        42.972f, 23.347f, 42.972f, 23.347f, 44.095f, 25.452f, 6.25f, 2.5f, 6.25f, 6.25f, 6.572f, 7.495f,
        7.989f, 8.896f, 12.578f, 11.523f, 13.428f, 12.505f, 13.428f, 12.505f, 13.75f, 13.75f, 13.428f, 14.995f,
        12.578f, 15.977f, 7.989f, 18.604f, 6.942f, 19.487f, 6.942f, 19.487f, 6.334f, 20.589f, 6.334f, 21.911f,
        6.942f, 23.013f, 12.578f, 26.523f, 13.428f, 27.505f, 13.428f, 27.505f, 13.75f, 28.75f, 13.428f, 29.995f,
        12.578f, 30.977f, 6.942f, 34.487f, 6.334f, 35.589f, 6.334f, 35.589f, 6.334f, 36.911f, 7.422f, 38.477f,
        12.578f, 41.523f, 13.428f, 42.505f, 13.75f, 43.75f, 62.5f, 17.5f, 62.537f, 17.5f, 17.5f, 62.5f,
        17.537f, 62.5f, 73.112f, 28.112f, 74.459f, 26.569f, 76.462f, 23.006f, 77.429f, 19.035f, 77.499f, 16.988f,
        76.805f, 12.96f, 76.805f, 12.96f, 76.053f, 11.054f, 75.05f, 9.269f, 72.363f, 6.188f, 70.731f, 4.95f,
        67.04f, 3.195f, 67.04f, 3.195f, 63.012f, 2.5f, 58.946f, 2.918f, 56.994f, 3.538f, 53.431f, 5.541f,
        51.887f, 6.887f, 51.887f, 6.887f, 6.887f, 51.888f, 5.541f, 53.431f, 3.538f, 56.994f, 2.918f, 58.947f,
        2.5f, 63.012f, 2.5f, 63.012f, 3.195f, 67.04f, 4.95f, 70.731f, 6.188f, 72.364f, 9.268f, 75.05f,
        12.96f, 76.805f, 12.96f, 76.805f, 14.95f, 77.29f, 19.035f, 77.43f, 23.006f, 76.462f, 26.568f, 74.459f,
        28.112f, 73.112f, 28.112f, 73.112f, 73.112f, 28.112f, 62.5f, 38.725f, 62.5f, 77.5f, 66.382f, 76.989f,
        70.0f, 75.49f, 71.631f, 74.4f, 73.106f, 73.107f, 73.106f, 73.107f, 75.49f, 70.0f, 76.989f, 66.382f,
        77.5f, 62.5f, 77.5f, 17.5f,
    };

    /**
     * @brief Grundformen des Symbols breather, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_BREATHER[] = {
        {IconShapeKind::Ring, false, {40.0f, 29.286f, 26.786f, 0.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Ring, false, {40.0f, 29.286f, 16.071f, 0.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 0, 5, false},
    };

    /**
     * @brief Grundformen des Symbols calendar, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_CALENDAR[] = {
        {IconShapeKind::RoundRect, false, {5.625f, 8.75f, 74.375f, 77.5f, 3.125f}, 0, 0, false},
        {IconShapeKind::Segment, false, {58.75f, 2.5f, 58.75f, 15.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {21.25f, 2.5f, 21.25f, 15.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {5.625f, 27.5f, 74.375f, 27.5f, 0.0f}, 0, 0, false},
    };

    /**
     * @brief Grundformen des Symbols config, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_CONFIG[] = {
        {IconShapeKind::Segment, false, {40.0f, 36.875f, 40.0f, 74.375f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {40.0f, 5.625f, 40.0f, 24.375f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {68.125f, 68.125f, 68.125f, 74.375f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {68.125f, 5.625f, 68.125f, 55.625f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {77.5f, 55.625f, 58.75f, 55.625f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {11.875f, 55.625f, 11.875f, 74.375f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {11.875f, 5.625f, 11.875f, 43.125f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {2.5f, 43.125f, 21.25f, 43.125f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {49.375f, 24.375f, 30.625f, 24.375f, 0.0f}, 0, 0, false},
    };

    /**
     * @brief Grundformen des Symbols contacts, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_CONTACTS[] = {
        {IconShapeKind::Ring, false, {40.0f, 29.062f, 25.0f, 0.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 5, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 11, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 17, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 23, 2, false},
    };

    /**
     * @brief Grundformen des Symbols dayview, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_DAYVIEW[] = {
        {IconShapeKind::RoundRect, false, {5.625f, 8.75f, 74.375f, 77.5f, 3.125f}, 0, 0, false},
        {IconShapeKind::Segment, false, {58.75f, 2.5f, 58.75f, 15.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {21.25f, 2.5f, 21.25f, 15.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {5.625f, 27.5f, 74.375f, 27.5f, 0.0f}, 0, 0, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 25, 3, false},
    };

    /**
     * @brief Grundformen des Symbols folder, Kachel 19 mal 16, Strichbreite 1.55
     */
    constexpr IconShape ICON_SHAPES_FOLDER[] = {
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 28, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 34, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 40, 3, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 43, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 49, 2, false},
    };

    /**
     * @brief Grundformen des Symbols folder_up, Kachel 17 mal 16, Strichbreite 1.40
     */
    constexpr IconShape ICON_SHAPES_FOLDER_UP[] = {
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 51, 3, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 54, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 60, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 66, 4, false},
    };

    /**
     * @brief Grundformen des Symbols mute, Kachel 28 mal 28, Strichbreite 1.90
     */
    constexpr IconShape ICON_SHAPES_MUTE[] = {
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 70, 6, true},
        {IconShapeKind::Arc, false, {14.0f, 14.0f, 5.5f, -52.0f, 52.0f}, 0, 0, false},
        {IconShapeKind::Arc, false, {14.0f, 14.0f, 9.0f, -58.0f, 58.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {5.0f, 5.0f, 23.0f, 23.0f, 0.0f}, 0, 0, false},
    };

    /**
     * @brief Grundformen des Symbols off, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_OFF[] = {
        {IconShapeKind::Segment, false, {40.0f, 4.248f, 40.0f, 38.339f, 0.0f}, 0, 0, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 76, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 82, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 88, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 94, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 100, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 106, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 112, 6, false},
    };

    /**
     * @brief Grundformen des Symbols player, Kachel 80 mal 80, gefüllte Fläche aus 15 Konturen, Aufdickung 0.50
     */
    constexpr IconShape ICON_SHAPES_PLAYER[] = {
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 118, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 125, 6, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 131, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 138, 6, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 144, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 151, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 158, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 165, 16, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 181, 9, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 190, 10, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 200, 6, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 206, 24, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 230, 21, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 251, 20, true},
        {IconShapeKind::FilledPolygon, false, {0.5f, 0, 0, 0, 0}, 271, 17, true},
    };

    /**
     * @brief Grundformen des Symbols pomodoro, Kachel 80 mal 80, gefüllte Fläche aus 15 Konturen, Aufdickung 0.70
     */
    constexpr IconShape ICON_SHAPES_POMODORO[] = {
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 288, 56, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 344, 28, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 372, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 379, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 386, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 393, 8, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 401, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 408, 6, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 414, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 421, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 428, 7, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 435, 62, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 497, 8, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 505, 4, true},
        {IconShapeKind::FilledPolygon, false, {0.7f, 0, 0, 0, 0}, 509, 46, true},
    };

    /**
     * @brief Grundformen des Symbols qrcode, Kachel 60 mal 60, Strichbreite 1.60
     */
    constexpr IconShape ICON_SHAPES_QRCODE[] = {
        {IconShapeKind::RoundRect, false, {1.6f, 6.0f, 10.56f, 14.96f, 1.12f}, 0, 0, false},
        {IconShapeKind::RoundRect, false, {1.6f, 19.44f, 10.56f, 28.4f, 1.12f}, 0, 0, false},
        {IconShapeKind::RoundRect, false, {15.04f, 6.0f, 24.0f, 14.96f, 1.12f}, 0, 0, false},
        {IconShapeKind::Segment, false, {15.04f, 19.44f, 15.04f, 23.92f, 0.0f}, 0, 0, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 555, 3, false},
        {IconShapeKind::Segment, false, {19.52f, 21.68f, 24.0f, 21.68f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {24.0f, 26.16f, 24.0f, 28.4f, 0.0f}, 0, 0, false},
        {IconShapeKind::RoundRect, false, {27.0f, 2.0f, 58.0f, 58.0f, 3.875f}, 0, 0, false},
        {IconShapeKind::Disc, false, {42.5f, 10.68f, 1.938f, 0.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::FilledRoundRect, false, {31.5f, 16.0f, 40.08f, 24.58f, 1.544f}, 0, 0, false},
        {IconShapeKind::FilledRoundRect, false, {31.5f, 29.42f, 40.08f, 38.0f, 1.544f}, 0, 0, false},
        {IconShapeKind::FilledRoundRect, false, {44.92f, 16.0f, 53.5f, 24.58f, 1.544f}, 0, 0, false},
        {IconShapeKind::FilledRoundRect, false, {44.92f, 29.42f, 53.5f, 38.0f, 1.544f}, 0, 0, false},
        {IconShapeKind::RoundRect, false, {36.0f, 45.5f, 49.0f, 50.0f, 2.25f}, 0, 0, false},
    };

    /**
     * @brief Grundformen des Symbols radio, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_RADIO[] = {
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 558, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 564, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 570, 4, false},
        {IconShapeKind::Segment, false, {2.5f, 24.375f, 65.0f, 5.625f, 0.0f}, 0, 0, false},
        {IconShapeKind::Ring, false, {52.5f, 49.375f, 12.5f, 0.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {15.0f, 36.875f, 27.5f, 36.875f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {15.0f, 49.375f, 27.5f, 49.375f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {15.0f, 61.875f, 27.5f, 61.875f, 0.0f}, 0, 0, false},
    };

    /**
     * @brief Grundformen des Symbols todo, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_TODO[] = {
        {IconShapeKind::Segment, false, {40.0f, 40.0f, 77.5f, 40.0f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {40.0f, 12.727f, 77.5f, 12.727f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {40.0f, 67.273f, 77.5f, 67.273f, 0.0f}, 0, 0, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 574, 3, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 577, 3, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 580, 3, false},
    };

    /**
     * @brief Grundformen des Symbols wheater, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_WHEATER[] = {
        {IconShapeKind::Segment, false, {25.181f, 15.418f, 23.673f, 6.849f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {13.955f, 22.57f, 6.828f, 17.578f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {11.069f, 35.565f, 2.5f, 37.077f, 0.0f}, 0, 0, false},
        {IconShapeKind::Segment, false, {38.18f, 18.303f, 43.167f, 11.177f, 0.0f}, 0, 0, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 583, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 589, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 595, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 601, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 607, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 613, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 619, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 625, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 631, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 637, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 643, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 649, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 655, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 661, 2, false},
    };

    /**
     * @brief Grundformen des Symbols workshop, Kachel 80 mal 80, Strichbreite 2.50
     */
    constexpr IconShape ICON_SHAPES_WORKSHOP[] = {
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 663, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 669, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 675, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 681, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 687, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 693, 2, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 695, 2, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 697, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 703, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 709, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 715, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 721, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 727, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 733, 2, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 735, 6, false},
        {IconShapeKind::Polyline, false, {0, 0, 0, 0, 0}, 741, 5, false},
    };

    /**
     * @brief Alle Symbole in der Reihenfolge der Aufzählung IconId
     */
    constexpr IconDefinition ICON_DEFINITIONS[] = {
        {ICON_SHAPES_BREATHER, 3, 80, 80, 2.5f},
        {ICON_SHAPES_CALENDAR, 4, 80, 80, 2.5f},
        {ICON_SHAPES_CONFIG, 9, 80, 80, 2.5f},
        {ICON_SHAPES_CONTACTS, 5, 80, 80, 2.5f},
        {ICON_SHAPES_DAYVIEW, 5, 80, 80, 2.5f},
        {ICON_SHAPES_FOLDER, 5, 19, 16, 1.55f},
        {ICON_SHAPES_FOLDER_UP, 4, 17, 16, 1.4f},
        {ICON_SHAPES_MUTE, 4, 28, 28, 1.9f},
        {ICON_SHAPES_OFF, 8, 80, 80, 2.5f},
        {ICON_SHAPES_PLAYER, 15, 80, 80, 0.0f},
        {ICON_SHAPES_POMODORO, 15, 80, 80, 0.0f},
        {ICON_SHAPES_QRCODE, 14, 60, 60, 1.6f},
        {ICON_SHAPES_RADIO, 8, 80, 80, 2.5f},
        {ICON_SHAPES_TODO, 6, 80, 80, 2.5f},
        {ICON_SHAPES_WHEATER, 18, 80, 80, 2.5f},
        {ICON_SHAPES_WORKSHOP, 16, 80, 80, 2.5f},
    };

    /**
     * @brief Groesste Formenzahl eines einzelnen Symbols
     *
     * Der Zeichner legt dafür die umschließenden Rechtecke auf dem Stapel ab.
     */
    constexpr uint8_t ICON_MAX_SHAPE_COUNT = 18;

    static_assert(sizeof(ICON_DEFINITIONS) / sizeof(ICON_DEFINITIONS[0]) == static_cast<size_t>(IconId::Count),
                  "Zu jedem Wert von IconId muss genau eine Symbolbeschreibung gehören");
}

#endif // ICON_SHAPE_DATA_H
