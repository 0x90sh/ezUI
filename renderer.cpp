#include "renderer.hpp"
#include "ezui.hpp"

DX11Renderer::DX11Renderer(HWND hwnd) : hwnd(hwnd) {}

DX11Renderer::~DX11Renderer() {
    if (renderTargetView) renderTargetView->Release();
    if (swapChain) swapChain->Release();
    if (d3dContext) d3dContext->Release();
    if (d3dDevice) d3dDevice->Release();
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (inputLayout) inputLayout->Release();
}

void DX11Renderer::initD3D11() {
    if (!hwnd) {
        ezUI::dbg("Invalid window handle (hwnd)!");
        return;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &swapChain,
        &d3dDevice,
        &featureLevel,
        &d3dContext
    );

    if (FAILED(hr)) {
        ezUI::dbg("Failed to create D3D11 device and swap chain! HRESULT: " + std::to_string(hr));
        return;
    }

    createRenderTarget();
    createBlendState();
    createVertexBuffer(1024);
    createShaders();

    D3D11_VIEWPORT viewport = {};
    RECT rect;
    GetClientRect(hwnd, &rect);
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<float>(rect.right - rect.left);
    viewport.Height = static_cast<float>(rect.bottom - rect.top);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    d3dContext->RSSetViewports(1, &viewport);
}

void DX11Renderer::createRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);

    if (FAILED(hr)) {
        ezUI::dbg("Failed to get the swap chain's back buffer! HRESULT: " + std::to_string(hr));
        return;
    }

    hr = d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();

    if (FAILED(hr)) {
        ezUI::dbg("Failed to create render target view! HRESULT: " + std::to_string(hr));
        return;
    }

    d3dContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
}

void DX11Renderer::createBlendState() {
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* blendState = nullptr;
    HRESULT hr = d3dDevice->CreateBlendState(&blendDesc, &blendState);

    if (FAILED(hr)) {
        ezUI::dbg("Failed to create blend state! HRESULT: " + std::to_string(hr));
        return;
    }

    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    d3dContext->OMSetBlendState(blendState, blendFactor, 0xffffffff);
    blendState->Release();
}

void DX11Renderer::createVertexBuffer(size_t vertexCount, size_t indexCount) {
    if (vertexBuffer) {
        vertexBuffer->Release();
        vertexBuffer = nullptr;
    }
    if (indexCount > 0 && indexBuffer) {
        indexBuffer->Release();
        indexBuffer = nullptr;
    }

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertexCount);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = d3dDevice->CreateBuffer(&vertexBufferDesc, nullptr, &vertexBuffer);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to create vertex buffer! HRESULT: " + std::to_string(hr));
        return;
    }

    if (indexCount > 0) {
        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * indexCount);
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = d3dDevice->CreateBuffer(&indexBufferDesc, nullptr, &indexBuffer);
        if (FAILED(hr)) {
            ezUI::dbg("Failed to create index buffer! HRESULT: " + std::to_string(hr));
            return;
        }
    }
}

void DX11Renderer::clearScreen(float r, float g, float b, float a) {
    float color[4] = { r, g, b, a };
    d3dContext->ClearRenderTargetView(renderTargetView, color);
}

void DX11Renderer::present() {
    swapChain->Present(1, 0);
}

void DX11Renderer::drawRectangle(float x, float y, float width, float height, const Color& color) {
    if (!d3dDevice || !d3dContext) {
        ezUI::dbg("Device or Context not initialized!");
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    float left = (2.0f * x / (rect.right - rect.left)) - 1.0f;
    float right = (2.0f * (x + width) / (rect.right - rect.left)) - 1.0f;
    float top = 1.0f - (2.0f * y / (rect.bottom - rect.top));
    float bottom = 1.0f - (2.0f * (y + height) / (rect.bottom - rect.top));

    Vertex vertices[] = {
        { XMFLOAT3(left, top, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) },
        { XMFLOAT3(right, top, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) },
        { XMFLOAT3(left, bottom, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) },
        { XMFLOAT3(right, bottom, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) }
    };

    createVertexBuffer(4);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = d3dContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to map vertex buffer! HRESULT: " + std::to_string(hr));
        return;
    }

    memcpy(mappedResource.pData, vertices, sizeof(vertices));
    d3dContext->Unmap(vertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    d3dContext->Draw(4, 0);
}

void DX11Renderer::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color) {
    if (!d3dDevice || !d3dContext) {
        ezUI::dbg("Device or Context not initialized!");
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    float nx1 = (2.0f * x1 / (rect.right - rect.left)) - 1.0f;
    float ny1 = 1.0f - (2.0f * y1 / (rect.bottom - rect.top));
    float nx2 = (2.0f * x2 / (rect.right - rect.left)) - 1.0f;
    float ny2 = 1.0f - (2.0f * y2 / (rect.bottom - rect.top));
    float nx3 = (2.0f * x3 / (rect.right - rect.left)) - 1.0f;
    float ny3 = 1.0f - (2.0f * y3 / (rect.bottom - rect.top));

    Vertex vertices[] = {
        { XMFLOAT3(nx1, ny1, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) },
        { XMFLOAT3(nx2, ny2, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) },
        { XMFLOAT3(nx3, ny3, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) }
    };

    createVertexBuffer(3);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = d3dContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to map vertex buffer! HRESULT: " + std::to_string(hr));
        return;
    }

    memcpy(mappedResource.pData, vertices, sizeof(vertices));
    d3dContext->Unmap(vertexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3dContext->Draw(3, 0);
}

void DX11Renderer::createShaders() {
    const char* vsSource = R"(
    struct VS_INPUT {
        float3 position : POSITION;
        float4 color : COLOR;
    };

    struct PS_INPUT {
        float4 position : SV_POSITION;
        float4 color : COLOR;
    };

    PS_INPUT main(VS_INPUT input) {
        PS_INPUT output;
        output.position = float4(input.position, 1.0);
        output.color = input.color;
        return output;
    }
    )";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            ezUI::dbg("Vertex Shader Compilation Error: " + std::string((char*)errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        return;
    }

    hr = d3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to create vertex shader! HRESULT: " + std::to_string(hr));
        vsBlob->Release();
        return;
    }

    d3dContext->VSSetShader(vertexShader, nullptr, 0);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = d3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    vsBlob->Release();

    if (FAILED(hr)) {
        ezUI::dbg("Failed to create input layout! HRESULT: " + std::to_string(hr));
        return;
    }

    d3dContext->IASetInputLayout(inputLayout);

    const char* psSource = R"(
    struct PS_INPUT {
        float4 position : SV_POSITION;
        float4 color : COLOR;
    };

    float4 main(PS_INPUT input) : SV_TARGET {
        return input.color;
    }
    )";

    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            ezUI::dbg("Pixel Shader Compilation Error: " + std::string((char*)errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        return;
    }

    hr = d3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    psBlob->Release();

    if (FAILED(hr)) {
        ezUI::dbg("Failed to create pixel shader! HRESULT: " + std::to_string(hr));
        return;
    }

    d3dContext->PSSetShader(pixelShader, nullptr, 0);
}

void DX11Renderer::drawCircle(float centerX, float centerY, float radius, const Color& color, int segments) {
    if (!d3dDevice || !d3dContext) {
        ezUI::dbg("Device or Context not initialized!");
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    float screenWidth = static_cast<float>(rect.right - rect.left);
    float screenHeight = static_cast<float>(rect.bottom - rect.top);

    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    float xCenterNorm = (2.0f * centerX / screenWidth) - 1.0f;
    float yCenterNorm = 1.0f - (2.0f * centerY / screenHeight);
    vertices.push_back({ XMFLOAT3(xCenterNorm, yCenterNorm, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) });

    for (int i = 0; i <= segments; ++i) {
        float theta = (2.0f * XM_PI * i) / segments;
        float x = centerX + radius * cosf(theta);
        float y = centerY + radius * sinf(theta);
        float xNorm = (2.0f * x / screenWidth) - 1.0f;
        float yNorm = 1.0f - (2.0f * y / screenHeight);
        vertices.push_back({ XMFLOAT3(xNorm, yNorm, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) });
    }

    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    createVertexBuffer(vertices.size(), indices.size());

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = d3dContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to map vertex buffer! HRESULT: " + std::to_string(hr));
        return;
    }
    memcpy(mappedResource.pData, vertices.data(), sizeof(Vertex) * vertices.size());
    d3dContext->Unmap(vertexBuffer, 0);

    hr = d3dContext->Map(indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to map index buffer! HRESULT: " + std::to_string(hr));
        return;
    }
    memcpy(mappedResource.pData, indices.data(), sizeof(UINT) * indices.size());
    d3dContext->Unmap(indexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    d3dContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3dContext->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void DX11Renderer::drawRoundedRectangle(float x, float y, float width, float height, float radius, const Color& color, int segments) {
    if (!d3dDevice || !d3dContext) {
        ezUI::dbg("Device or Context not initialized!");
        return;
    }

    radius = min(radius, min(width, height) / 2.0f);

    RECT rect;
    GetClientRect(hwnd, &rect);
    float screenWidth = static_cast<float>(rect.right - rect.left);
    float screenHeight = static_cast<float>(rect.bottom - rect.top);

    std::vector<Vertex> vertices;
    std::vector<UINT> indices;

    float xCenterNorm = (2.0f * (x + width / 2.0f) / screenWidth) - 1.0f;
    float yCenterNorm = 1.0f - (2.0f * (y + height / 2.0f) / screenHeight);
    vertices.push_back({ XMFLOAT3(xCenterNorm, yCenterNorm, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) });

    float left = x + radius;
    float right = x + width - radius;
    float top = y + radius;
    float bottom = y + height - radius;

    int totalSegments = segments * 4;
    for (int i = 0; i <= totalSegments; ++i) {
        float theta = (XM_2PI * i) / totalSegments;

        float cosTheta = cosf(theta);
        float sinTheta = sinf(theta);

        float cornerX = 0.0f;
        float cornerY = 0.0f;

        if (theta >= 0 && theta < XM_PIDIV2) {
            cornerX = right;
            cornerY = bottom;
        }
        else if (theta >= XM_PIDIV2 && theta < XM_PI) {
            cornerX = left;
            cornerY = bottom;
        }
        else if (theta >= XM_PI && theta < 3 * XM_PIDIV2) {
            cornerX = left;
            cornerY = top;
        }
        else {
            cornerX = right;
            cornerY = top;
        }

        float xPos = cornerX + radius * cosTheta;
        float yPos = cornerY + radius * sinTheta;

        float xNorm = (2.0f * xPos / screenWidth) - 1.0f;
        float yNorm = 1.0f - (2.0f * yPos / screenHeight);

        vertices.push_back({ XMFLOAT3(xNorm, yNorm, 0.0f), XMFLOAT4(color.r, color.g, color.b, color.a) });
    }

    for (int i = 1; i <= totalSegments; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i == totalSegments ? 1 : i + 1);
    }

    createVertexBuffer(vertices.size(), indices.size());

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = d3dContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to map vertex buffer! HRESULT: " + std::to_string(hr));
        return;
    }
    memcpy(mappedResource.pData, vertices.data(), sizeof(Vertex) * vertices.size());
    d3dContext->Unmap(vertexBuffer, 0);

    hr = d3dContext->Map(indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        ezUI::dbg("Failed to map index buffer! HRESULT: " + std::to_string(hr));
        return;
    }
    memcpy(mappedResource.pData, indices.data(), sizeof(UINT) * indices.size());
    d3dContext->Unmap(indexBuffer, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    d3dContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3dContext->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void DX11Renderer::draw(const DrawCommand& command) {
    switch (command.type) {
    case SHAPE_RECTANGLE:
        if (command.shape.rectangle.rounding > 0.0f) {
            drawRoundedRectangle(command.shape.rectangle.x, command.shape.rectangle.y, command.shape.rectangle.width, command.shape.rectangle.height, command.shape.rectangle.rounding, command.shape.rectangle.color, 32);
        } else {
            drawRectangle(command.shape.rectangle.x, command.shape.rectangle.y, command.shape.rectangle.width, command.shape.rectangle.height, command.shape.rectangle.color);
        }
        break;
    case SHAPE_CIRCLE:
        drawCircle(command.shape.circle.centerX, command.shape.circle.centerY, command.shape.circle.radius, command.shape.circle.color, command.shape.circle.segments);
        break;
    case SHAPE_TRIANGLE:
        drawTriangle(command.shape.triangle.x1, command.shape.triangle.y1, command.shape.triangle.x2, command.shape.triangle.y2, command.shape.triangle.x3, command.shape.triangle.y3, command.shape.triangle.color);
        break;
    default:
        ezUI::dbg("Invalid shape type!");
        break;
    }
}

