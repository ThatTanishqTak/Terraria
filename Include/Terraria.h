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

internal void GameUpdateAndRender(game_Offscreen_Buffer* Buffer, int xOffset, int yOffset);

#define TERRARIA_H
#endif