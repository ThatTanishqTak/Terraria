// Include necessary Windows header file
#include <Windows.h>
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

global_variable bool running;

global_variable BITMAPINFO bitmapInfo;
global_variable void* bitmapMemory;

global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable int bytesPerPixel = 4;

internal void Rendering(int xOffset, int yOffset)
{
    int width = bitmapWidth;
    int height = bitmapHeight;

     int pitch = width * bytesPerPixel;

    uint8* row = (uint8*)bitmapMemory;
    for (int y = 0; y < bitmapHeight; ++y)
    {
        uint32* pixels = (uint32*)row;
        for (int x = 0; x < bitmapWidth; ++x)
        {
            uint8 blue = (x + xOffset);
            uint8 green = (y + yOffset);

            *pixels++ = ((green << 8) | blue);
        }

        row += pitch;
    }
}

internal void Win32_ResizeDIBSection(int width, int height)
{
    if (bitmapMemory)
    {
        VirtualFree(bitmapMemory, NULL, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    /*--------------------------------------------- 
       bitmapInfo.bmiHeader.biSizeImage     = 0;
       bitmapInfo.bmiHeader.biXPelsPerMeter = 0;                                                                   
       bitmapInfo.bmiHeader.biYPelsPerMeter = 0;       [[ "bitmapInfo" IS A STATIC VARIABLE SO ALL THESE ARE INITIALIZE TO 0 AUTOMATICALLY ]]
       bitmapInfo.bmiHeader.biClrUsed       = 0;
       bitmapInfo.bmiHeader.biClrImportant  = 0; 
    ----------------------------------------------*/


    int bitmapMemorySize = (bitmapWidth * bitmapHeight)* bytesPerPixel;

    bitmapMemory = VirtualAlloc(NULL, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32_UpdateWindow(HDC deviceContext, RECT* clientRect, int x, int y, int width, int height)
{
    int windowWidth = clientRect->right - clientRect->left;
    int windowHeight = clientRect->bottom - clientRect->top;

     StretchDIBits(deviceContext,                   // Active device context
                   0, 0, bitmapWidth, bitmapHeight, // Destination
                   0, 0, windowWidth, windowHeight, // Source
                   bitmapMemory,                    // Bitmap memory
                   &bitmapInfo,                     // Bitmap information
                   DIB_RGB_COLORS,                  // Color palate
                   SRCCOPY);                        // DWORD property
}

// Callback function for handling window messages
LRESULT CALLBACK Win32_MainWindowCallBack(HWND Window,   // Handles window
                                          UINT Message,  // The callback message (unsigned int)
                                          WPARAM WParam, // Word-Parameter (unsigned int pointer)
                                          LPARAM LParam) // Long-Parameter (long pointer)
{
    LRESULT result = 0; // Initialize result variable

    // Switch statement to handle various window messages
    switch (Message)
    {
        // Handle size change event
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(Window, &clientRect);

            int height = clientRect.bottom - clientRect.top; // Height of the area to be painted
            int width = clientRect.right - clientRect.left; // Width of the area to be painted

            Win32_ResizeDIBSection(width, height);
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
            running = false;
        }
        break;

        // Handle paint requests
        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct; // Structure containing information about painting
            HDC deviceContext = BeginPaint(Window, &PaintStruct); // Start painting

            int x = PaintStruct.rcPaint.left; // Left edge of the area to be painted
            int y = PaintStruct.rcPaint.top; // Top edge of the area to be painted
            int height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top; // Height of the area to be painted
            int width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left; // Width of the area to be painted
            local_persist DWORD operation = BLACKNESS; // Operation to perform (fill with black color)

            RECT clientRect;
            GetClientRect(Window, &clientRect);

            Win32_UpdateWindow(deviceContext, &clientRect, x, y, width, height);

            EndPaint(Window, &PaintStruct); // End painting
        }
        break;

        // Default handler for unhandled messages
        default:
        {
            //OutputDebugStringA("default\n"); // Debug output
            result = DefWindowProc(Window, Message, WParam, LParam); // Call the default window procedure
        }
        break;
    }

    return result; // Return the result of the message handling
}

// Entry point for a Windows application
int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CommandLine, int ShowCode)
{
    WNDCLASS WindowClass = {}; // Initialize window class structure

    // Configure window class properties
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // Style flags
    WindowClass.lpfnWndProc = Win32_MainWindowCallBack; // Pointer to window procedure
    WindowClass.hInstance = Instance; // Instance handle
    //WindowClass.hIcon = Icon; // Uncomment and initialize if needed
    WindowClass.lpszClassName = L"Terraria_Window_Class"; // Class name

    // Register the window class
    if (RegisterClass(&WindowClass))
    {
        // Create the window
        HWND Window = CreateWindowEx(NULL,                                // Set DWORD ExStyle
                                     WindowClass.lpszClassName,           // Long pointer to the class name
                                     L"Terraria-Clone",                   // Window title
                                     WS_OVERLAPPEDWINDOW | WS_VISIBLE,    // DWORD properties
                                     CW_USEDEFAULT,                       // X-Position
                                     CW_USEDEFAULT,                       // Y-Position
                                     CW_USEDEFAULT,                       // Width
                                     CW_USEDEFAULT,                       // Height
                                     NULL,                                // Set parent
                                     NULL,                                // Set menu
                                     Instance,                            // Active window instance
                                     NULL);                               // Long pointer to a void

        if (Window) // Check if window creation was successful
        {
            int xOffset = 0;
            int yOffset = 0;

            // Main message loop
            running = true;
            while (running)
            {

                MSG message; // Message structure
                while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE))
                {
                    if (message.message == WM_QUIT) { running = false; }

                    TranslateMessage(&message); // Translate virtual-key messages into character messages
                    DispatchMessage(&message); // Dispatches a message to a window procedure
                }

                Rendering(xOffset, yOffset);

                HDC deviceContext = GetDC(Window);

                RECT clientRect;
                GetClientRect(Window, &clientRect);
                int windowWidth = clientRect.right - clientRect.left;
                int windowHeight = clientRect.bottom - clientRect.top;
                Win32_UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
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