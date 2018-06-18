#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define MATH_D3D
#include <core/math.h>
#include <core/coredefs.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

static IDXGISwapChain* gSwapchain;
static ID3D11Device* gDevice;
static ID3D11DeviceContext* gDC;
static ID3D11RenderTargetView* gBackBuffer;
static ID3D11VertexShader* gVertexShader;
static ID3D11PixelShader* gPixelShader;

static const vec4f_t gClearColor= { 0.f, 0.5f, 1.f, 1.f };

static void d3dResizeFrame(int w, int h)
{
	ID3D11Texture2D* pBackBuffer = NULL;
	gSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (gBackBuffer)
	{
		gBackBuffer->Release();
	}
	gDevice->CreateRenderTargetView(pBackBuffer, NULL, &gBackBuffer);
	pBackBuffer->Release();
	gDC->OMSetRenderTargets(1, &gBackBuffer, NULL);
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)w;
	viewport.Height = (float)h;
	gDC->RSSetViewports(1, &viewport);
}

static void d3dInitialize(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
	scd.BufferCount = 2;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 4;
#ifdef PROGRAM_FULLSCREEN
	scd.Windowed = FALSE;
#else
	scd.Windowed = TRUE;
#endif
	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &scd, &gSwapchain, &gDevice, &level, &gDC);
	RECT windowRect;
	GetClientRect(hWnd, &windowRect);
	int w = (windowRect.right - windowRect.left) & INT_MAX;
	int h = (windowRect.bottom - windowRect.top) & INT_MAX;
	d3dResizeFrame(w, h);
	ID3D10Blob *vsBlob = NULL, *psBlob = NULL;
	ENSURE(D3DReadFileToBlob(L"vertex_shader.cso", &vsBlob) == S_OK);
	ENSURE(D3DReadFileToBlob(L"pixel_shader.cso", &psBlob) == S_OK);
	gDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &gVertexShader);
	gDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &gPixelShader);
	gDC->VSSetShader(gVertexShader, 0, 0);
	gDC->PSSetShader(gPixelShader, 0, 0);
}

static void d3dRenderFrame()
{
	gDC->ClearRenderTargetView(gBackBuffer, gClearColor);
	gDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gDC->Draw(3, 0);
	gSwapchain->Present(0, 0);
}

static void d3dShutdown()
{
	gVertexShader->Release();
	gPixelShader->Release();
	gSwapchain->Release();
	gBackBuffer->Release();
	gDevice->Release();
	gDC->Release();
}

#ifndef PROGRAM_FULLSCREEN
static void onWindowResized(GLFWwindow* window, int width, int height)
{
	d3dResizeFrame(width, height);
}
#endif

int CALLBACK WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    if (glfwInit())
    {
        GLFWmonitor* monitor = nullptr;
        int width = 1024, height = 768;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef PROGRAM_FULLSCREEN
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        width = vidMode->width;
        height = vidMode->height;
#else
		glfwWindowHint(GLFW_MAXIMIZED, 1);
#endif
        GLFWwindow* window = glfwCreateWindow(width, height, PROGRAM_NAME, monitor, nullptr);
        if (window)
        {
			d3dInitialize(glfwGetNativeView(window));
#ifndef PROGRAM_FULLSCREEN
			glfwSetFramebufferSizeCallback(window, &onWindowResized);
#endif
            while (!glfwWindowShouldClose(window))
            {
				d3dRenderFrame();
                glfwPollEvents();
            }
			d3dShutdown();
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    return 0;
}
