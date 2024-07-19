#if !defined TERRARIA_H

// Structure that contains data about the buffer
struct game_Offscreen_Buffer
{
    void* Memory;

    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_Sound_Output_Buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16* Samples;
};

internal void GameUpdateAndRender(game_Offscreen_Buffer* Buffer, int xOffset, int yOffset, game_Sound_Output_Buffer* SoundBuffer);

#define TERRARIA_H
#endif