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
static ID3D11Buffer* gGlobalParamsBuffer;

static struct
{
	mat4f_t projection, view;
} gGlobalParams;

static const struct vertex_t
{
	vec3f_t position, normal;
	vec2f_t texCoord;
} gQuadMeshData[] = 
{
	{ { -1.f,  1.f,0.f },{ 0.f,0.f,-1.f },{ 0.f,0.f } },
	{ {  1.f,  1.f,0.f },{ 0.f,0.f,-1.f },{ 1.f,0.f } },
	{ { -1.f, -1.f,0.f },{ 0.f,0.f,-1.f },{ 0.f,1.f } },
	{ { -1.f, -1.f,0.f },{ 0.f,0.f,-1.f },{ 0.f,1.f } },
	{ {  1.f,  1.f,0.f },{ 0.f,0.f,-1.f },{ 1.f,0.f } },
	{ {  1.f, -1.f,0.f },{ 0.f,0.f,-1.f },{ 1.f,1.f } },
};

static const vec4f_t gClearColor = { 0.f, 0.5f, 1.f, 1.f };


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
	D3D11_MAPPED_SUBRESOURCE res;
	gDC->Map(gGlobalParamsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	memcpy(res.pData, &gGlobalParams, sizeof(gGlobalParams));
	gDC->Unmap(gGlobalParamsBuffer, 0);
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
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(gGlobalParams);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ENSURE(!FAILED(gDevice->CreateBuffer(&bd, NULL, &gGlobalParamsBuffer)));
	d3dResizeFrame(w, h);
	ID3D10Blob *vsBlob = NULL, *psBlob = NULL;
	ENSURE(D3DReadFileToBlob(L"default_vs.cso", &vsBlob) == S_OK);
	ENSURE(D3DReadFileToBlob(L"default_ps.cso", &psBlob) == S_OK);
	gDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &gVertexShader);
	gDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &gPixelShader);
	gDC->VSSetShader(gVertexShader, 0, 0);
	gDC->PSSetShader(gPixelShader, 0, 0);
	gDC->VSSetConstantBuffers(0, 1, &gGlobalParamsBuffer);
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
	gGlobalParamsBuffer->Release();
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
	mathMat4fPerspective(gGlobalParams.projection, mathDeg2Rad(90.f), width / ((float)height), 0.01f, 1000.f);
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
			vec3f_t worldOrigin {0.f, 0.f, 1.f};
			glfwGetWindowSize(window, &width, &height);
			mathMat4fIdentity(gGlobalParams.view);
			mathMat4fTranslate(gGlobalParams.view, worldOrigin);
			mathMat4fPerspective(gGlobalParams.projection, mathDeg2Rad(90.f), width / ((float)height), 0.01f, 1000.f);
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
