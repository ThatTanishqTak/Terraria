// Include necessary Windows header file
#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdint.h>

#define internal static;
#define local_persist static;
#define global_variable static;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32_t bool32;

struct Win32_Offscreen_Buffer
{
    BITMAPINFO Info;

    void* Memory;

    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct Win32_Window_Dimension
{
    int Width;
    int Height;
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

typedef X_INPUT_GET_STATE(X_InputGetState);
typedef X_INPUT_SET_STATE(X_InputSetState);
typedef DIRECT_SOUND_CREATE(Direct_Sound_Create);

X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }

global_variable X_InputGetState* XInputGetState_ = XInputGetStateStub;
global_variable X_InputSetState* XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

global_variable bool running;
global_variable Win32_Offscreen_Buffer globalBackBuffer;

internal void Win32_LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (XInputLibrary)
    {
        XInputGetState = (X_InputGetState*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (X_InputSetState*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else {  } // Error
}

internal void Win32_InitDirectSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // Load the library
    HMODULE DirectSoundLibrary = LoadLibraryA("dsound.dll");
    if (DirectSoundLibrary)
    {
        // Get a DirectSound object
        Direct_Sound_Create *DirectSoundCreate = (Direct_Sound_Create*)GetProcAddress(DirectSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND Direct_Sound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(NULL, &Direct_Sound, NULL)))
        {
            WAVEFORMATEX WaveFormat = {};

            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(Direct_Sound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {  };
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
                    else {  } // Error
                }
                else {  } // Error
            }
            else {  } // Error

            DSBUFFERDESC BufferDescription = {  };
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = NULL;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            // "Create" a secondary buffer
            LPDIRECTSOUNDBUFFER SecondaryBuffer;
            if (SUCCEEDED(Direct_Sound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, NULL)))
            {
                OutputDebugStringA("Secondary Success\n");
            }
            else {  } // Error

            // Start it playing
        }
        else {  } // Error
    }
}

internal Win32_Window_Dimension Win32_GetWindowDimension(HWND Window)
{
    Win32_Window_Dimension result;

    RECT clientRect;
    GetClientRect(Window, &clientRect);

    result.Height = clientRect.bottom - clientRect.top;  // Height of the area to be painted
    result.Width = clientRect.right - clientRect.left;   // Width of the area to be painted

    return result;
}

internal void Render(Win32_Offscreen_Buffer* buffer, int xOffset, int yOffset)
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

internal void Win32_ResizeDIBSection(Win32_Offscreen_Buffer* buffer, int width, int height)
{
    if (buffer->Memory)
    {
        VirtualFree(buffer->Memory,  // Address to where the memory is
                    NULL,            // Size of the memory to the region to be freed (If NULL means the entire block)
                    MEM_RELEASE);    // Releases the specified region of pages
    }

    buffer->Width         = width;
    buffer->Height        = height;
    buffer->BytesPerPixel = 4;

    buffer->Info.bmiHeader.biSize        = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth       = buffer->Width;
    buffer->Info.bmiHeader.biHeight      = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes      = 1;
    buffer->Info.bmiHeader.biBitCount    = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    /*----------------------------------------------//
       bitmapInfo.bmiHeader.biSizeImage     = 0;
       bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
       bitmapInfo.bmiHeader.biYPelsPerMeter = 0;       [[ "bitmapInfo" IS A STATIC VARIABLE SO ALL THESE ARE INITIALIZE TO 0 AUTOMATICALLY ]]
       bitmapInfo.bmiHeader.biClrUsed       = 0;
       bitmapInfo.bmiHeader.biClrImportant  = 0;
    //----------------------------------------------*/

    int bitmapMemorySize = (buffer->Width * buffer->Height) * buffer->BytesPerPixel;

    buffer->Memory = VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->Pitch = width * buffer->BytesPerPixel;
}

internal void Win32_DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, Win32_Offscreen_Buffer *buffer, int x, int y, int width, int height)
{
    StretchDIBits(deviceContext,                        // Active device context
                  0, 0, windowWidth, windowHeight,      // Destination rect
                  0, 0, buffer->Width, buffer->Height,  // Source rect
                  buffer->Memory,                       // Bitmap memory
                  &buffer->Info,                        // Bitmap information
                  DIB_RGB_COLORS,                       // Color palate
                  SRCCOPY);                             // DWORD property
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
        case WM_ACTIVATEAPP: { OutputDebugStringA("WM_ACTIVEAPP\n"); } // Debug output
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
           bool WasDown = (LParam & (1 << 30)) != 0;
           bool IsDown  = (LParam & (1 << 31)) == 0;

           if (WasDown != IsDown)
           {
               if (VKCode == 'W')
               {

               }
               else if (VKCode == 'A')
               {

               }
               else if (VKCode == 'S')
               {

               }
               else if (VKCode == 'D')
               {

               }
               else if (VKCode == 'Q')
               {

               }
               else if (VKCode == 'E')
               {

               }
               else if (VKCode == VK_UP)
               {

               }
               else if (VKCode == VK_LEFT)
               {

               }
               else if (VKCode == VK_DOWN)
               {

               }
               else if (VKCode == VK_RIGHT)
               {

               }
               else if (VKCode == VK_SPACE)
               {

               }
               else if (VKCode == VK_ESCAPE)
               {

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

            int x                         = PaintStruct.rcPaint.left;                              // Left edge of the area to be painted
            int y                         = PaintStruct.rcPaint.top;                               // Top edge of the area to be painted
            int height                    = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;  // Height of the area to be painted
            int width                     = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;  // Width of the area to be painted
            local_persist DWORD operation = BLACKNESS;                                             // Operation to perform (fill with black color)

            Win32_Window_Dimension dimension = Win32_GetWindowDimension(Window);
            Win32_DisplayBufferInWindow(deviceContext, dimension.Width, dimension.Height, &globalBackBuffer, x, y, width, height);

            EndPaint(Window, &PaintStruct); // End painting
        }
        break;

        // Default handler for unhandled messages
        default:
        {
            result = DefWindowProc(Window, Message, WParam, LParam); // Call the default window procedure
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

            int xOffset = 0;
            int yOffset = 0;

            Win32_InitDirectSound(Window, 48000, 48000 * sizeof(int16) * 2);

            // Main message loop
            running = true;
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

                        bool Up            = (pad->wButtons && XINPUT_GAMEPAD_DPAD_UP);
                        bool Down          = (pad->wButtons && XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left          = (pad->wButtons && XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right         = (pad->wButtons && XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start         = (pad->wButtons && XINPUT_GAMEPAD_START);
                        bool Back          = (pad->wButtons && XINPUT_GAMEPAD_BACK);
                        bool LeftThumb     = (pad->wButtons && XINPUT_GAMEPAD_LEFT_THUMB);
                        bool RightThumb    = (pad->wButtons && XINPUT_GAMEPAD_RIGHT_THUMB);
                        bool LeftShoulder  = (pad->wButtons && XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (pad->wButtons && XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton       = (pad->wButtons && XINPUT_GAMEPAD_A);
                        bool BButton       = (pad->wButtons && XINPUT_GAMEPAD_B);
                        bool XButton       = (pad->wButtons && XINPUT_GAMEPAD_X);
                        bool YButton       = (pad->wButtons && XINPUT_GAMEPAD_Y);

                        int16 StickX = pad->sThumbLX;
                        int16 StickY = pad->sThumbLY;
                    }
                    else
                    {
                        // The controller is not available
                    }
                }

                Render(&globalBackBuffer, xOffset, yOffset);

                HDC deviceContext = GetDC(Window);
                Win32_Window_Dimension dimension = Win32_GetWindowDimension(Window);

                Win32_DisplayBufferInWindow(deviceContext, dimension.Width, dimension.Height, &globalBackBuffer, 0, 0, dimension.Width, dimension.Height);
                ReleaseDC(Window, deviceContext);

                xOffset++;
                yOffset++;
            }
        }
        else {  } // Handle error if window creation fails
    }
    else {  } // Handle error if window class registration fails

    return 0; // Return 0 to indicate successful execution
}