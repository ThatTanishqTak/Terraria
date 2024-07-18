#include "../Include/Terraria.h"

internal void Render(game_Offscreen_Buffer* buffer, int xOffset, int yOffset)
{
    uint8* row = (uint8*)buffer->Memory;
    for (int y = 0; y < buffer->Height; ++y)
    {
        uint32* pixels = (uint32*)row;
        for (int x = 0; x < buffer->Width; ++x)
        {
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset);

            *pixels++ = ((green << 8) | blue);
        }

        row += buffer->Pitch;
    }
}

internal void GameUpdateAndRender(game_Offscreen_Buffer* Buffer, int xOffset, int yOffset)
{
    Render(Buffer, xOffset, yOffset);
}