#pragma once
#include "renderer.hpp"
#include <functional>
#include <unordered_map>
#include <chrono>
#include <vector>

#ifdef _DEBUG
#define EZUI_DEBUG 1
#else
#define EZUI_DEBUG 0
#endif

class ezUI {
public:
    ezUI(DX11Renderer& renderer) : renderer(renderer) {
        registerDefaultStyles();
    }

    static void dbg(const std::string& message) {
#if EZUI_DEBUG
        std::cout << "[dbg] " << message << std::endl;
#endif
    }

    struct Style {
        std::function<std::vector<DX11Renderer::DrawCommand>(const DX11Renderer::Rectangle&, DX11Renderer::Color)> createCommands;

        Style() {}

        Style(std::function<std::vector<DX11Renderer::DrawCommand>(const DX11Renderer::Rectangle&, DX11Renderer::Color)> createCommands)
            : createCommands(createCommands) {}
    };

    struct Container {
        std::string name;
        std::string styleName;
        DX11Renderer::Rectangle bounds;
        bool visible;
        float paddingX;
        float paddingY;
        float maxWidth;
        float maxHeight;
        float currentHeight;
        float currentWidth;

        Container()
            : name(""), styleName("defaultContainer"), bounds(0.0f, 0.0f, 100.0f, 100.0f, 0.0f, DX11Renderer::Color(0.0f, 0.0f, 0.0f, 1.0f)),
            visible(false), paddingX(10.0f), paddingY(10.0f), maxWidth(500.0f), maxHeight(500.0f),
            currentHeight(0.0f), currentWidth(0.0f) {}
        Container(const std::string& name, float x, float y, float width, float height, DX11Renderer::Color color = DX11Renderer::Color(0.0f, 0.0f, 0.0f, 1.0f), const std::string& style = "defaultContainer", float paddingX = 10.0f, float paddingY = 10.0f, float maxWidth = 500.0f, float maxHeight = 500.0f)
            : name(name), styleName(style), bounds(x, y, width, height, 0.0f, color), visible(false), paddingX(paddingX), paddingY(paddingY), maxWidth(maxWidth), maxHeight(maxHeight), currentHeight(0.0f), currentWidth(0.0f) {}
    };

    struct Button {
        std::string containername;
        std::string name;
        std::string styleName;
        DX11Renderer::Rectangle bounds;
        std::function<void(Button&)> onClick;
        std::function<void(Button&)> onHover;
        std::function<void(Button&)> onIdle;

        Button()
            : containername(""), styleName("defaultButton"), bounds(0.0f, 0.0f, 100.0f, 30.0f, 0.0f, DX11Renderer::Color(0, 0, 0, 0)),
            onClick(nullptr), onHover(nullptr), onIdle(nullptr) {}

        Button(const std::string& containername, const std::string& name, DX11Renderer::Rectangle bounds, std::function<void(Button&)> clickCallback = nullptr, std::function<void(Button&)> hoverCallback = nullptr, std::function<void(Button&)> idleCallback = nullptr, const std::string& style = "defaultButton")
            : containername(containername), name(name), styleName(style), bounds(bounds),
            onClick(clickCallback), onHover(hoverCallback), onIdle(idleCallback) {}
    };

    struct Hotkey {
        std::string containername;
        int virtualKey;
        std::function<void()> onKeyPress;
        std::chrono::time_point<std::chrono::steady_clock> lastUseTime;
        int rateLimitMs;

        Hotkey()
            : containername(""), virtualKey(0), onKeyPress(nullptr), lastUseTime(std::chrono::steady_clock::now()), rateLimitMs(250) {}
        Hotkey(const std::string& containername, int vk, std::function<void()> callback, int rateLimitMs = 250)
            : containername(containername), virtualKey(vk), onKeyPress(callback), lastUseTime(std::chrono::steady_clock::now()), rateLimitMs(rateLimitMs) {}
    };

    void registerStyle(const std::string& styleName, const Style& style) {
        if (styles.find(styleName) != styles.end()) {
            dbg("Style with name '" + styleName + "' already exists. Skipping registration.");
            return;
        }
        styles[styleName] = style;
    }

    void addContainer(const std::string& name, float x, float y, float width, float height, DX11Renderer::Color color = DX11Renderer::Color(0.0f, 0.0f, 0.0f, 1.0f), const std::string& style = "defaultContainer", float paddingX = 10.0f, float paddingY = 10.0f, float maxWidth = 500.0f, float maxHeight = 500.0f) {
        if (containers.find(name) != containers.end()) {
            dbg("Container with name '" + name + "' already exists. Skipping addition.");
            return;
        }
        containers[name] = Container(name, x, y, width, height, color, style, paddingX, paddingY, maxWidth, maxHeight);
    }

    void addButton(const std::string& containername, const std::string& name, DX11Renderer::Rectangle bounds, std::function<void(Button&)> clickCallback = nullptr, std::function<void(Button&)> hoverCallback = nullptr, std::function<void(Button&)> idleCallback = nullptr, const std::string& style = "defaultButton") {       
        if (buttons.find(name) != buttons.end()) {
            dbg("Button with name '" + name + "' already exists. Skipping addition.");
            return;
        }

        auto containerIt = containers.find(containername);
        if (containerIt == containers.end()) {
            dbg("Container '" + containername + "' not found. Cannot add button.");
            return;
        }

        Container& container = containerIt->second;
        bounds.x += container.bounds.x;
        bounds.y += container.bounds.y;
        
        buttons[name] = Button(containername, name, bounds, clickCallback, hoverCallback, idleCallback, style);
    }

    void addHotkey(const std::string& containername, int virtualKey, std::function<void()> callback, int rateLimitMs = 250) {
        if (hotkeys.find(virtualKey) != hotkeys.end()) {
            dbg("Hotkey with virtual key '" + std::to_string(virtualKey) + "' already exists. Skipping addition.");
            return;
        }
        hotkeys[virtualKey] = Hotkey(containername, virtualKey, callback, rateLimitMs);
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
            if (isContainerVisible(button.containername)) {
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
            // Draw containers
            for (auto& containerPair : containers) {
                Container& container = containerPair.second;
                if (container.visible) {
                    applyStyle(container.styleName, container.bounds, container.bounds.color);
                }
            }

            // Draw buttons
            for (auto& buttonPair : buttons) {
                Button& button = buttonPair.second;
                if (isContainerVisible(button.containername)) {
                    applyStyle(button.styleName, button.bounds, button.bounds.color);
                }
            }
        }

        renderer.present();
    }


    void masterToggle() {
        masterSwitch = !masterSwitch;
    }

    bool isContainerVisible(const std::string& containername) const {
        if (!masterSwitch) return false;
        auto containerIt = containers.find(containername);
        if (containerIt != containers.end()) {
            return containerIt->second.visible;
        }
        else {
            dbg("Container not found: " + containername);
        }
        return false;
    }

    void toggleVisibility(const std::string& containername) {
        auto containerIt = containers.find(containername);
        if (containerIt != containers.end()) {
            containerIt->second.visible = !containerIt->second.visible;
        }
        else {
            dbg("Container not found: " + containername);
        }
    }

    void registerDefaultStyles() {
        registerStyle("defaultContainer", Style([](const DX11Renderer::Rectangle& bounds, DX11Renderer::Color accentColor) {
            return std::vector<DX11Renderer::DrawCommand>{
                DX11Renderer::DrawCommand::CreateRectangle(bounds.x, bounds.y, bounds.width, bounds.height, bounds.rounding, accentColor)
            };
        }));

        registerStyle("defaultButton", Style([](const DX11Renderer::Rectangle& bounds, DX11Renderer::Color accentColor) {
            return std::vector<DX11Renderer::DrawCommand>{
                DX11Renderer::DrawCommand::CreateRectangle(bounds.x, bounds.y, bounds.width, bounds.height, bounds.rounding, accentColor)
            };
        }));
    }


private:
    DX11Renderer& renderer;
    std::unordered_map<std::string, Container> containers;
    std::unordered_map<std::string, Button> buttons;
    std::unordered_map<int, Hotkey> hotkeys;
    std::unordered_map<std::string, Style> styles;

    std::chrono::time_point<std::chrono::steady_clock> lastClickTime;
    bool masterSwitch = true;

    bool isMouseOver(const DX11Renderer::Rectangle& bounds, int mouseX, int mouseY) {
        return mouseX >= bounds.x && mouseX <= (bounds.x + bounds.width) && mouseY >= bounds.y && mouseY <= (bounds.y + bounds.height);
    }

    void applyStyle(const std::string& styleName, const DX11Renderer::Rectangle& bounds, const DX11Renderer::Color& accentColor) {
        auto styleIt = styles.find(styleName);
        if (styleIt != styles.end()) {
            std::vector<DX11Renderer::DrawCommand> commands = styleIt->second.createCommands(bounds, accentColor);
            for (const auto& command : commands) {
                renderer.draw(command);
            }
        }
        else {
            dbg("Style not found: " + styleName);
        }
    }

};
