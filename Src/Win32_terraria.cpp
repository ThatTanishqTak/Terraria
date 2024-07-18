// Windows header file
#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdint.h>
#include <math.h>

// Macros to differentiate between all the statics
#define internal static;
#define local_persist static;
#define global_variable static;
constexpr auto PI32 = 3.14159265359f;

// typedef to ease using some of the other types of ints and uints
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32_t bool32;

typedef float real32;
typedef double real64;

// Game header files
#include "Terraria.cpp"

// Structure that contains data about the buffer
struct Win32_Offscreen_Buffer
{
    BITMAPINFO Info;
    void* Memory;

    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

// Structure that contains the window dimensions
struct Win32_Window_Dimension
{
    int Width;
    int Height;
};

// In order to prevent linking the entire xinput library, 
// I have created these macros to help in defining the two functions
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

// Similarly in the case of dsound library.
// But here I have done this as many machines do not have direct sound in them,
// So just define the relevant function here
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

// Using typedef to ease in function creation
typedef X_INPUT_GET_STATE(X_InputGetState);
typedef X_INPUT_SET_STATE(X_InputSetState);
typedef DIRECT_SOUND_CREATE(Direct_Sound_Create);

// This is here for error handling purposes. 
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }

// Global variables for functions
global_variable X_InputGetState* XInputGetState_ = XInputGetStateStub;
global_variable X_InputSetState* XInputSetState_ = XInputSetStateStub;

// Defining macros for the stub function
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

// Global variables to be used through out the program
global_variable bool32 running;
global_variable Win32_Offscreen_Buffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER SecondaryAudioBuffer;

// Static function to load the xinput library
internal void Win32_LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary) { XInputLibrary = LoadLibraryA("xinput9_1_0.dll"); } // If version 1.4 is not avaiable
    if (!XInputLibrary) { XInputLibrary = LoadLibraryA("xinput1_3.dll"); } // If version 1.4 is not avaiable

    if (XInputLibrary)
    {
        XInputGetState = (X_InputGetState*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (X_InputSetState*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else {} // Error
}

// Static function to initialize DirectSound
internal void Win32_InitDirectSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // Load the library
    HMODULE DirectSoundLibrary = LoadLibraryA("dsound.dll");
    if (DirectSoundLibrary)
    {
        // Get a DirectSound object
        Direct_Sound_Create* DirectSoundCreate = (Direct_Sound_Create*)GetProcAddress(DirectSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND Direct_Sound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(NULL, &Direct_Sound, NULL)))
        {
            WAVEFORMATEX WaveFormat = {  }; // Initialize WaveFormat structure to NULL

            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(Direct_Sound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {  }; // Initialize the BufferDescription struct to NULL
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // "Create" a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(Direct_Sound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, NULL)))
                {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        OutputDebugStringA("Primary Success\n");
                    }
                    else {} // Error
                }
                else {} // Error
            }
            else {} // Error

            DSBUFFERDESC BufferDescription = {  };
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = NULL;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            // "Create" a secondary buffer
            if (SUCCEEDED(Direct_Sound->CreateSoundBuffer(&BufferDescription, &SecondaryAudioBuffer, NULL)))
            {
                OutputDebugStringA("Secondary Success\n");
            }
            else {} // Error

            // Start it playing
        }
        else {} // Error
    }
}

internal Win32_Window_Dimension Win32_GetWindowDimension(HWND Window)
{
    Win32_Window_Dimension result = {};

    RECT clientRect;
    GetClientRect(Window, &clientRect);

    result.Height = clientRect.bottom - clientRect.top;  // Height of the area to be painted
    result.Width = clientRect.right - clientRect.left;   // Width of the area to be painted

    return result;
}

internal void Win32_ResizeDIBSection(Win32_Offscreen_Buffer* buffer, int width, int height)
{
    if (buffer->Memory)
    {
        VirtualFree(buffer->Memory,  // Address to where the memory is
            NULL,            // Size of the memory to the region to be freed (If NULL means the entire block)
            MEM_RELEASE);    // Releases the specified region of pages
    }

    buffer->Width = width;
    buffer->Height = height;
    buffer->BytesPerPixel = 4;

    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (buffer->Width * buffer->Height) * buffer->BytesPerPixel;

    buffer->Memory = VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->Pitch = width * buffer->BytesPerPixel;
}

internal void Win32_DisplayBufferInWindow(HDC deviceContext,
                                          Win32_Offscreen_Buffer* buffer,
                                          int windowWidth, int windowHeight,
                                          int x,
                                          int y,
                                          int destinationWidth, int destinationHeight)
{
    StretchDIBits(deviceContext,                        // Active device context
                  0, 0, windowWidth, windowHeight,      // Destination rect
                  0, 0, buffer->Width, buffer->Height,  // Source rect
                  buffer->Memory,                       // Bitmap memory
                  &buffer->Info,                        // Bitmap information
                  DIB_RGB_COLORS,                       // Color palate
                  SRCCOPY);                             // DWORD property
}

struct Win32_Sound_Output
{
    // This is for sound test
    int SamplesPerSeconds;
    int TonesHz;
    int16 ToneVolume;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
    real32 tSine;
};

internal void Win32_FillSoundBuffer(Win32_Sound_Output* SoundOutput, DWORD BytesToLock, DWORD BytesToWrite)
{
    // Variables to store data into the secondary buffer
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;

    // Lock the secondary buffer
    if (SUCCEEDED(SecondaryAudioBuffer->Lock(BytesToLock,             // The amount of bytes to lock
                                             BytesToWrite,            // The amount of bytes available to write
                                             &Region1, &Region1Size,  // Pointer to the first region and it's size
                                             &Region2, &Region2Size,  // Pointer to the second region and it's size
                                             NULL)))                  // Addition flag
    {

        DWORD Region1SampleCounter = Region1Size / SoundOutput->BytesPerSample; // Calculate the number of samples needed for the first region based on its size and bytes per sample
        int16* SampleOut = (int16*)Region1; // Cast the pointer to the start of Region1 to an int16 pointer, which will be used to store the output samples

        // Loop through each sample in Region1
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCounter; SampleIndex++)
        {
            real32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            SoundOutput->tSine += 2.0f * PI32 * 1.0f / (real32)SoundOutput->WavePeriod;

            // Store the calculated sample value at the current position in Region1
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            ++SoundOutput->RunningSampleIndex;
        }

        // Do the same for region2
        DWORD Region2SampleCounter = Region2Size / SoundOutput->BytesPerSample;
        SampleOut = (int16*)Region2;

        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCounter; SampleIndex++)
        {
            real32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f * PI32 * 1.0f / (real32)SoundOutput->WavePeriod;
            ++SoundOutput->RunningSampleIndex;
        }

        // Unlock the secondary buffer
        SecondaryAudioBuffer->Unlock(Region1,  // Long pointer to the first region
                                     Region1Size,   // Size of the first pointer
                                     Region2,       // Long pointer to the second region
                                     Region2Size);  // Size of the second pointer
    }
}

// Callback function for handling window messages
internal LRESULT CALLBACK Win32_MainWindowCallBack(HWND Window,    // Handles window
                                                   UINT Message,   // The callback message (unsigned int)
                                                   WPARAM WParam,  // Word-Parameter (unsigned int pointer)
                                                   LPARAM LParam)  // Long-Parameter (long pointer)
{
    LRESULT result = 0; // Initialize result variable

    // Switch statement to handle various window messages
    switch (Message)
    {
        // Handle size change event
    case WM_SIZE:
    {

    }
    break;

    // Handle close request
    case WM_CLOSE:
    {
        running = false;
    }
    break;

    // Handle application activation/deactivation
    case WM_ACTIVATEAPP:
    {
        // Debug output
        OutputDebugStringA("WM_ACTIVEAPP\n");
    }
    break;

    // Handle window destroy event
    case WM_DESTROY:
    {
        // Error handling and maybe restart the window as this should only be called when the window is closed due to an error
        running = false;
    }
    break;

    // keyboard inputs
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32 VKCode = WParam;
        bool32 WasDown = (LParam & (1 << 30)) != 0;
        bool32 IsDown = (LParam & (1 << 31)) == 0;

        if (WasDown != IsDown)
        {
            // All the keys that I will use for keyboard input
            switch (VKCode)
            {
            case 'W':
            {

            }
            break;

            case 'A':
            {

            }
            break;

            case 'S':
            {

            }
            break;

            case 'D':
            {

            }
            break;

            case 'Q':
            {

            }
            break;

            case 'E':
            {

            }
            break;

            case VK_UP:
            {

            }
            break;

            case VK_LEFT:
            {

            }
            break;

            case VK_DOWN:
            {

            }
            break;

            case VK_RIGHT:
            {

            }
            break;

            case VK_SPACE:
            {

            }
            break;

            case VK_ESCAPE:
            {

            }
            break;

            default:
            {

            }
            break;
            }

        }

        bool32 AltWasDown = (LParam & (1 << 29));
        if ((VKCode == VK_F4) && AltWasDown)
        {
            running = false;
        }
    }
    break;

    // Handle paint requests
    case WM_PAINT:
    {
        PAINTSTRUCT PaintStruct; // Structure containing information about painting
        HDC deviceContext = BeginPaint(Window, &PaintStruct); // Start painting

        int x = PaintStruct.rcPaint.left;                                   // Left edge of the area to be painted
        int y = PaintStruct.rcPaint.top;                                    // Top edge of the area to be painted
        int height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;  // Height of the area to be painted
        int width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;   // Width of the area to be painted
        local_persist DWORD operation = BLACKNESS;                          // Operation to perform (fill with black color)

        Win32_Window_Dimension dimension = Win32_GetWindowDimension(Window);

        // This function takes the buffer and displays it onto the screen
        Win32_DisplayBufferInWindow(deviceContext,      // Handle to the device context 
                                    &globalBackBuffer,  // Reference to the global back buffer
                                    dimension.Width,    // Window width
                                    dimension.Height,   // Window height
                                    0,                  // X-Position
                                    0,                  // Y-Position
                                    dimension.Width,    // Destination width
                                    dimension.Height);  // Destination height

        EndPaint(Window, &PaintStruct); // End painting
    }
    break;

    // Default handler for unhandled messages
    default:
    {
        // Call the default window procedure, This call handles all messages that are not explicitly handled by the above switch case.
        result = DefWindowProc(Window,   // Handles window
                               Message,  // The callback message (unsigned int)
                               WParam,   // Word-Parameter (unsigned int pointer)
                               LParam);  // Long-Parameter (long pointer)
    }
    break;
    }

    return result; // Return the result of the message handling
}

// Entry point for a Windows application
int WINAPI WinMain(HINSTANCE Instance,      // Handle to the instance
                   HINSTANCE PrevInstance,  // Pervious handle to the instance (WAS USED IN 16-BIT WINDOWS, NOW IS ALWAYS NULL)
                   PSTR CommandLine,        // The command-line argument as an Unicode string
                   int ShowCode)            // Will be used when doing error handling
{
    LARGE_INTEGER PerformanceCounterFrequencyResult;
    QueryPerformanceFrequency(&PerformanceCounterFrequencyResult);
    int64 PerformanceCounterFrequency = PerformanceCounterFrequencyResult.QuadPart;

    Win32_LoadXInput();

    WNDCLASSW WindowClass = {}; // Initialize window class structure

    Win32_ResizeDIBSection(&globalBackBuffer, 1280, 720);

    // Configure window class properties
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;  // Style flags
    WindowClass.lpfnWndProc = Win32_MainWindowCallBack;      // Pointer to window procedure
    WindowClass.hInstance = Instance;                        // Instance handle
    //WindowClass.hIcon = Icon;                              // Uncomment and initialize when needed
    WindowClass.lpszClassName = L"Terraria_Window_Class";    // Class name

    // Register the window class
    if (RegisterClassW(&WindowClass))
    {
        // Create the window
        HWND Window = CreateWindowExW(NULL,                             // Optional window style
                                      WindowClass.lpszClassName,         // Long pointer to the class name
                                      L"Terraria-Clone",                 // Window title
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,  // Window style
                                      CW_USEDEFAULT,                     // X-Position
                                      CW_USEDEFAULT,                     // Y-Position
                                      CW_USEDEFAULT,                     // Width
                                      CW_USEDEFAULT,                     // Height
                                      NULL,                              // Set parent window
                                      NULL,                              // Set menu
                                      Instance,                          // Active window instance
                                      NULL);                             // Additional application data (Long void pointer)

        if (Window) // Check if window creation was successful
        {
            HDC deviceContext = GetDC(Window);

            // This is for graphics test
            int xOffset = 0;
            int yOffset = 0;

            Win32_Sound_Output SoundOutput = {};

            SoundOutput.SamplesPerSeconds = 48000;
            SoundOutput.TonesHz = 256;
            SoundOutput.ToneVolume = 3000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSeconds / SoundOutput.TonesHz;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSeconds * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSeconds / 15;

            Win32_InitDirectSound(Window,                            // Active window
                                  SoundOutput.SamplesPerSeconds,     // The samples hertz
                                  SoundOutput.SecondaryBufferSize);  // The size of the secondary buffer

            Win32_FillSoundBuffer(&SoundOutput,
                                  0,
                                  SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample);

            SecondaryAudioBuffer->Play(NULL,              // Must be NULL
                                       NULL,              // Must be NULL
                                       DSBPLAY_LOOPING);  // Flag that loops the sound

            running = true;

            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);

            // Main message loop
            int64 LastCycleCount = __rdtsc();
            while (running)
            {
                MSG message; // Message structure
                while (PeekMessage(&message,    // Long-pointer to the message structure
                                   NULL,        // Handle to the current window (NULL means it will receive message from any window)
                                   NULL,        // For minimum message range (NULL means there is no range)
                                   NULL,        // For maximum message range (NULL means there is no range)
                                   PM_REMOVE))  // A flag so that messages are removed from the queue after processing by PeekMessage
                {
                    if (message.message == WM_QUIT) { running = false; }

                    TranslateMessage(&message); // Translate virtual-key messages into character messages
                    DispatchMessageW(&message); // Dispatches a message to a window procedure
                }

                for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex)
                {
                    XINPUT_STATE controllerState;
                    if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // The controller is plugged in
                        XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

                        // All the inputs that I will use for a gamepad
                        bool32 Up            = (pad->wButtons && XINPUT_GAMEPAD_DPAD_UP);
                        bool32 Down          = (pad->wButtons && XINPUT_GAMEPAD_DPAD_DOWN);
                        bool32 Left          = (pad->wButtons && XINPUT_GAMEPAD_DPAD_LEFT);
                        bool32 Right         = (pad->wButtons && XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool32 Start         = (pad->wButtons && XINPUT_GAMEPAD_START);
                        bool32 Back          = (pad->wButtons && XINPUT_GAMEPAD_BACK);
                        bool32 LeftThumb     = (pad->wButtons && XINPUT_GAMEPAD_LEFT_THUMB);
                        bool32 RightThumb    = (pad->wButtons && XINPUT_GAMEPAD_RIGHT_THUMB);
                        bool32 LeftShoulder  = (pad->wButtons && XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool32 RightShoulder = (pad->wButtons && XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool32 AButton       = (pad->wButtons && XINPUT_GAMEPAD_A);
                        bool32 BButton       = (pad->wButtons && XINPUT_GAMEPAD_B);
                        bool32 XButton       = (pad->wButtons && XINPUT_GAMEPAD_X);
                        bool32 YButton       = (pad->wButtons && XINPUT_GAMEPAD_Y);

                        // This is for the left analog stick
                        int16 StickX = pad->sThumbLX;
                        int16 StickY = pad->sThumbLY;
                    }
                    else
                    {
                        // The controller is not available
                        // Show a message to the player that the controller is not connected
                    }
                }
                
                game_Offscreen_Buffer Buffer = {};
                Buffer.Memory = globalBackBuffer.Memory;
                Buffer.Width = globalBackBuffer.Width;
                Buffer.Height = globalBackBuffer.Height;
                Buffer.Pitch = globalBackBuffer.Pitch;
                GameUpdateAndRender(&Buffer, xOffset, yOffset);

                // DirectSound output test
                DWORD PlayCursor; // This will read from the secondary buffer and play the sound
                DWORD WriteCursor; // This will write to the secondary buffer and will be ahead of the PlayCursor

                if (SUCCEEDED(SecondaryAudioBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                    DWORD TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
                    DWORD BytesToWrite = 0;

                    if (BytesToLock > TargetCursor)
                    {
                        BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
                        BytesToWrite += TargetCursor;
                    }
                    else
                    {
                        BytesToWrite = TargetCursor - BytesToLock;
                    }

                    Win32_FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite);
                }

                Win32_Window_Dimension dimension = Win32_GetWindowDimension(Window); // Set the window dimension in it's own variable for easy access

                // This function takes the buffer and displays it onto the screen
                Win32_DisplayBufferInWindow(deviceContext,      // Handle to the device context 
                                            &globalBackBuffer,  // Reference to the global back buffer
                                            dimension.Width,    // Window width
                                            dimension.Height,   // Window height
                                            0,                  // X-Position
                                            0,                  // Y-Position
                                            dimension.Width,    // Destination width
                                            dimension.Height);  // Destination height

                int64 EndCycleCount = __rdtsc();;

                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);

                // Display the value here
                int64 CycleElapsed = EndCycleCount - LastCycleCount;
                int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                int32 MSPerFrame = (int32)(1000 * CounterElapsed) / PerformanceCounterFrequency;
                int32 FramesPerSeconds = PerformanceCounterFrequency / CounterElapsed;
                int32 MegaCyclesPerSeconds = CycleElapsed / (1000 * 1000);
#if 0
                char Buffer[256];
                wsprintfA(Buffer, "%dMilliSeconds/Frames, %dFrames/Second, %dMegaCycles/Frame\n", MSPerFrame, FramesPerSeconds, MegaCyclesPerSeconds);
                OutputDebugStringA(Buffer);
#endif
                LastCounter = EndCounter;
                LastCycleCount = EndCycleCount;
            }
        }
        else {} // Handle error if window creation fails
    }
    else {} // Handle error if window class registration fails

    return 0; // Return 0 to indicate successful execution
}