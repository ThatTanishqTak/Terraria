#include "../Include/Terraria.h"

internal void GameOutputSound(game_Sound_Output_Buffer* SoundBuffer)
{
    local_persist real32 tSine{};
    int16 ToneVolume = 3000;
    int ToneHz = 256;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16* SampleOut = SoundBuffer->Samples;

    // Loop through each sample in Region1
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);

        // Store the calculated sample value at the current position in Region1
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += ((2.0f * PI32) * (1.0f / (real32)WavePeriod));
    }
}

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

internal void GameUpdateAndRender(game_Offscreen_Buffer* Buffer, int xOffset, int yOffset, game_Sound_Output_Buffer* SoundBuffer)
{
    GameOutputSound(SoundBuffer);
    Render(Buffer, xOffset, yOffset);
}