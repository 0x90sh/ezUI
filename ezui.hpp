#pragma once
#include "renderer.hpp"
#include <functional>
#include <unordered_map>
#include <chrono>

#ifdef _DEBUG
#define EZUI_DEBUG 1
#else
#define EZUI_DEBUG 0
#endif

class ezUI {
public:
    ezUI(DX11Renderer& renderer) : renderer(renderer) {}

    static void dbg(const std::string& message) {
#if EZUI_DEBUG
        std::cout << "[dbg] " << message << std::endl;
#endif
    }

    struct Button {
        std::string name;
        DX11Renderer::Rectangle bounds;
        std::function<void(Button&)> onClick;
        std::function<void(Button&)> onHover;
        std::function<void(Button&)> onIdle;

        Button() : bounds(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DX11Renderer::Color(0, 0, 0, 0)) {}
        Button(const std::string& name, DX11Renderer::Rectangle bounds, std::function<void(Button&)> clickCallback, std::function<void(Button&)> hoverCallback, std::function<void(Button&)> idleCallback = nullptr)
            : name(name), bounds(bounds), onClick(clickCallback), onHover(hoverCallback), onIdle(idleCallback) {}
    };

    struct Slider {
        std::string name;
        DX11Renderer::Rectangle trackBounds;
        DX11Renderer::Rectangle thumbBounds;
        float minValue;
        float maxValue;
        float* value;
        std::function<void(Slider&)> onValueChanged;
        std::function<void(Slider&)> onIdle;

        Slider() : trackBounds(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DX11Renderer::Color(0, 0, 0, 0)), thumbBounds(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DX11Renderer::Color(0, 0, 0, 0)), minValue(0), maxValue(0), value(nullptr), onIdle(nullptr) {}
        Slider(const std::string& name, DX11Renderer::Rectangle trackBounds, DX11Renderer::Rectangle thumbBounds, float minValue, float maxValue, float* value, std::function<void(Slider&)> valueChangedCallback, std::function<void(Slider&)> idleCallback = nullptr)
            : name(name), trackBounds(trackBounds), thumbBounds(thumbBounds), minValue(minValue), maxValue(maxValue), value(value), onValueChanged(valueChangedCallback), onIdle(idleCallback) {}
    };

    struct InputBox {
        std::string name;
        DX11Renderer::Rectangle bounds;
        std::string text;
        std::function<void(InputBox&)> onTextChanged;
        std::function<void(InputBox&)> onIdle;

        InputBox() : bounds(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DX11Renderer::Color(0, 0, 0, 0)), onIdle(nullptr) {}
        InputBox(const std::string& name, DX11Renderer::Rectangle bounds, const std::string& text, std::function<void(InputBox&)> textChangedCallback, std::function<void(InputBox&)> idleCallback = nullptr)
            : name(name), bounds(bounds), text(text), onTextChanged(textChangedCallback), onIdle(idleCallback) {}
    };

    struct Checkbox {
        std::string name;
        DX11Renderer::Rectangle bounds;
        bool checked;
        std::function<void(Checkbox&)> onCheckedChanged;
        std::function<void(Checkbox&)> onIdle;

        Checkbox() : bounds(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DX11Renderer::Color(0, 0, 0, 0)), checked(false), onIdle(nullptr) {}
        Checkbox(const std::string& name, DX11Renderer::Rectangle bounds, bool initialValue, std::function<void(Checkbox&)> checkedChangedCallback, std::function<void(Checkbox&)> idleCallback = nullptr)
            : name(name), bounds(bounds), checked(initialValue), onCheckedChanged(checkedChangedCallback), onIdle(idleCallback) {}
    };

    struct ColorPicker {
        std::string name;
        DX11Renderer::Rectangle bounds;
        DX11Renderer::Color* selectedColor;
        std::function<void(ColorPicker&)> onColorChanged;
        std::function<void(ColorPicker&)> onIdle;

        ColorPicker() : bounds(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, DX11Renderer::Color(0, 0, 0, 0)), selectedColor(nullptr), onIdle(nullptr) {}
        ColorPicker(const std::string& name, DX11Renderer::Rectangle bounds, DX11Renderer::Color* selectedColor, std::function<void(ColorPicker&)> colorChangedCallback, std::function<void(ColorPicker&)> idleCallback = nullptr)
            : name(name), bounds(bounds), selectedColor(selectedColor), onColorChanged(colorChangedCallback), onIdle(idleCallback) {}
    };

    struct Hotkey {
        int virtualKey;
        std::function<void()> onKeyPress;
        std::chrono::time_point<std::chrono::steady_clock> lastUseTime;
        int rateLimitMs;

        Hotkey()
            : virtualKey(0), onKeyPress(nullptr), lastUseTime(std::chrono::steady_clock::now()), rateLimitMs(250) {}
        Hotkey(int vk, std::function<void()> callback, int rateLimitMs = 250)
            : virtualKey(vk), onKeyPress(callback), lastUseTime(std::chrono::steady_clock::now()), rateLimitMs(rateLimitMs) {}
    };

    void addButton(const std::string& name, DX11Renderer::Rectangle bounds, std::function<void(Button&)> clickCallback, std::function<void(Button&)> hoverCallback, std::function<void(Button&)> idleCallback = nullptr) {
        buttons[name] = Button(name, bounds, clickCallback, hoverCallback, idleCallback);
    }

    void addHotkey(int virtualKey, std::function<void()> callback, int rateLimitMs = 250) {
        hotkeys[virtualKey] = Hotkey(virtualKey, callback, rateLimitMs);
    }

    void handleInput() {
        POINT mousePos;
        GetCursorPos(&mousePos);
        int mouseX = mousePos.x;
        int mouseY = mousePos.y;
        bool mouseLeftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float, std::milli> elapsed = currentTime - lastClickTime;

        for (auto& buttonPair : buttons) {
            Button& button = buttonPair.second;
            if (isMouseOver(button.bounds, mouseX, mouseY)) {
                if (mouseLeftDown && elapsed.count() > 250) {
                    lastClickTime = currentTime;
                    if (button.onClick) button.onClick(button);
                }
                else if (button.onHover) {
                    button.onHover(button);
                }
            }
            else if (button.onIdle) {
                button.onIdle(button);
            }
        }

        for (auto& hotkeyPair : hotkeys) {
            Hotkey& hotkey = hotkeyPair.second;
            if (GetAsyncKeyState(hotkey.virtualKey) & 0x8000) {
                auto timeSinceLastUse = std::chrono::steady_clock::now() - hotkey.lastUseTime;
                if (std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceLastUse).count() > hotkey.rateLimitMs) {
                    hotkey.lastUseTime = std::chrono::steady_clock::now();
                    if (hotkey.onKeyPress) {
                        hotkey.onKeyPress();
                    }
                }
            }
        }

    }

    void drawAllElements() {
        renderer.clearScreen(0.0f, 0.0f, 0.0f, 0.0f);

        if (masterSwitch) {
            for (auto& buttonPair : buttons) {
                Button& button = buttonPair.second;
                renderer.draw(DX11Renderer::DrawCommand::CreateRectangle(button.bounds.x, button.bounds.y, button.bounds.width, button.bounds.height, 0.0f, button.bounds.color));
            }
        }

        renderer.present();
    }

    void masterToggle() {
        if (masterSwitch) {
            masterSwitch = false;
        }
        else {
            masterSwitch = true;
        }
    }

private:
    DX11Renderer& renderer;

    std::unordered_map<std::string, Button> buttons;
    std::unordered_map<std::string, Slider> sliders;
    std::unordered_map<std::string, InputBox> inputBoxes;
    std::unordered_map<std::string, Checkbox> checkboxes;
    std::unordered_map<std::string, ColorPicker> colorPickers;
    std::unordered_map<int, Hotkey> hotkeys;

    std::chrono::time_point<std::chrono::steady_clock> lastClickTime;

    bool masterSwitch = true;

    bool isMouseOver(const DX11Renderer::Rectangle& bounds, int mouseX, int mouseY) {
        return mouseX >= bounds.x && mouseX <= (bounds.x + bounds.width) && mouseY >= bounds.y && mouseY <= (bounds.y + bounds.height);
    }
};
