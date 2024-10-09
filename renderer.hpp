#pragma once
#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <algorithm>
#include <dwmapi.h>
#include <vector>
#include <map>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;


class DX11Renderer {
public:
    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };
    struct Color {
        float r, g, b, a;
        Color(float red, float green, float blue, float alpha) : r(red), g(green), b(blue), a(alpha) {}
    };

    struct Rectangle {
        float x, y, width, height;
        float rounding;
        Color color;

        Rectangle(float x, float y, float width, float height, float rounding, Color color)
            : x(x), y(y), width(width), height(height), rounding(rounding), color(color) {}
    };

    struct Circle {
        float centerX, centerY, radius;
        Color color;
        int segments;

        Circle(float centerX, float centerY, float radius, Color color, int segments = 64)
            : centerX(centerX), centerY(centerY), radius(radius), color(color), segments(segments) {}
    };

    struct Triangle {
        float x1, y1, x2, y2, x3, y3;
        Color color;

        Triangle(float x1, float y1, float x2, float y2, float x3, float y3, Color color)
            : x1(x1), y1(y1), x2(x2), y2(y2), x3(x3), y3(y3), color(color) {}
    };

    union Shape {
        Rectangle rectangle;
        Circle circle;
        Triangle triangle;

        Shape() {}
        ~Shape() {}
    };

    enum ShapeType {
        SHAPE_RECTANGLE,
        SHAPE_CIRCLE,
        SHAPE_TRIANGLE
    };

    struct DrawCommand {
        ShapeType type;
        Shape shape;

        DrawCommand() : type(SHAPE_RECTANGLE) {}

        static DrawCommand CreateRectangle(float x, float y, float width, float height, float rounding, Color color) {
            DrawCommand command;
            command.type = SHAPE_RECTANGLE;
            command.shape.rectangle = Rectangle(x, y, width, height, rounding, color);
            return command;
        }

        static DrawCommand CreateCircle(float centerX, float centerY, float radius, Color color, int segments = 48) {
            DrawCommand command;
            command.type = SHAPE_CIRCLE;
            command.shape.circle = Circle(centerX, centerY, radius, color, segments);
            return command;
        }

        static DrawCommand CreateTriangle(float x1, float y1, float x2, float y2, float x3, float y3, Color color) {
            DrawCommand command;
            command.type = SHAPE_TRIANGLE;
            command.shape.triangle = Triangle(x1, y1, x2, y2, x3, y3, color);
            return command;
        }
    };

    struct Element {
        std::string name;
        int priority;
        std::vector<DrawCommand> commands;

        Element() : name(""), priority(0), commands({}) {}

        Element(const std::string& name, int priority, const std::vector<DrawCommand>& commands)
            : name(name), priority(priority), commands(commands) {}
    };


    DX11Renderer(HWND hwnd);
    ~DX11Renderer();

    void registerElement(const std::string& name, int priority, const std::vector<DrawCommand>& commands);
    void drawAllElements();
    void drawElement(const std::string& name);
    void clearElements();
    void draw(const DrawCommand& command);
    void initD3D11();
    void clearScreen(float r, float g, float b, float a);
    void present();
    void drawRectangle(float x, float y, float width, float height, const Color& color);
    void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color);
    void drawCircle(float centerX, float centerY, float radius, const Color& color, int segments = 64);
    void drawRoundedRectangle(float x, float y, float width, float height, float radius, const Color& color, int segments = 64);

private:
    HWND hwnd;
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
    ID3D11Buffer* vertexBuffer = nullptr;
    std::map<std::string, Element> elements;

    void createRenderTarget();
    void createBlendState();
    void createVertexBuffer(size_t vertexCount, size_t indexCount = 0);
    void createShaders();

    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
};