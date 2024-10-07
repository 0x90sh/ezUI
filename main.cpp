#include "renderer.hpp"
using DrawCommand = DX11Renderer::DrawCommand;
using Color = DX11Renderer::Color;

class WindowHijacker {
public:
    HWND findWindow(const std::string& partialName);
    HWND createWindow(const std::string& windowName, int width, int height, DWORD style = WS_POPUP | WS_VISIBLE | WS_EX_TOPMOST | WS_EX_LAYERED);

private:
    struct SearchData {
        std::string name;
        HWND hwnd;
    };
    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);
    std::string toLower(const std::string& str);
};

HWND WindowHijacker::findWindow(const std::string& partialName) {
    SearchData searchData;
    searchData.name = toLower(partialName);
    searchData.hwnd = nullptr;

    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&searchData));
    return searchData.hwnd;
}

BOOL CALLBACK WindowHijacker::enumWindowsProc(HWND hwnd, LPARAM lParam) {
    char windowTitle[256];
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

    std::string titleStr = windowTitle;
    std::transform(titleStr.begin(), titleStr.end(), titleStr.begin(), ::tolower);

    SearchData* searchData = reinterpret_cast<SearchData*>(lParam);
    if (titleStr.find(searchData->name) != std::string::npos) {
        searchData->hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

std::string WindowHijacker::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

HWND WindowHijacker::createWindow(const std::string& windowName, int width, int height, DWORD style) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ezUIwindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        std::cerr << "Failed to register window class! Error: " << GetLastError() << std::endl;
        return nullptr;
    }

    std::wstring wWindowName(windowName.begin(), windowName.end());

    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        wWindowName.c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!hwnd) {
        std::cerr << "Failed to create window! Error: " << GetLastError() << std::endl;
        return nullptr;
    }

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

int main() {
    WindowHijacker hijacker;
    HWND hwnd = hijacker.findWindow("yourwindowtohijack");

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (!hwnd) {
        std::cout << "Window not found, creating a new window.\n";
        hwnd = hijacker.createWindow("My ezUI Window", screenWidth, screenHeight);
    }

    DX11Renderer renderer(hwnd);
    renderer.initD3D11();

    renderer.registerElement("TestElement", 0, { DrawCommand::CreateRectangle(100.0f, 100.0f, 200.0f, 150.0f, 0.0f, Color(1.0f, 0.0f, 0.0f, 1.0f)), DrawCommand::CreateRectangle(400.0f, 100.0f, 200.0f, 150.0f, 20.0f, Color(0.0f, 1.0f, 0.0f, 1.0f)), DrawCommand::CreateCircle(300.0f, 300.0f, 50.0f, Color(0.0f, 0.0f, 1.0f, 1.0f), 64), DrawCommand::CreateTriangle(500.0f, 500.0f, 600.0f, 500.0f, 550.0f, 600.0f, Color(1.0f, 1.0f, 0.0f, 1.0f)) });
    renderer.registerElement("TestElement2", 0, { DrawCommand::CreateRectangle(800.0f, 800.0f, 200.0f, 150.0f, 0.0f, Color(0.0f, 0.0f, 1.0f, 1.0f)) });

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        renderer.clearScreen(0.2f, 0.2f, 0.2f, 0.3f);
        //renderer.drawAllElements();
        renderer.drawElement("TestElement");
        renderer.drawElement("TestElement2");
        renderer.draw(DrawCommand::CreateRectangle(650.0f, 1150.0f, 200.0f, 150.0f, 0.0f, Color(0.0f, 1.0f, 1.0f, 1.0f)));
        renderer.present();
    }

    return 0;
}
