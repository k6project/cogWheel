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
static ID3D11InputLayout* gQuadVertexLayout;
static ID3D11Buffer* gGlobalParamsBuffer;
static ID3D11Buffer* gQuadMeshVertexBuffer;

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
	{ { -1.f, 1.f,0.f },{ 0.f,0.f,-1.f },{ 0.f,0.f } },
	{ {  1.f, 1.f,0.f },{ 0.f,0.f,-1.f },{ 1.f,0.f } },
	{ { -1.f,-1.f,0.f },{ 0.f,0.f,-1.f },{ 0.f,1.f } },
	{ { -1.f,-1.f,0.f },{ 0.f,0.f,-1.f },{ 0.f,1.f } },
	{ {  1.f, 1.f,0.f },{ 0.f,0.f,-1.f },{ 1.f,0.f } },
	{ {  1.f,-1.f,0.f },{ 0.f,0.f,-1.f },{ 1.f,1.f } },
};

static const vec4f_t gClearColor = { 0.f, 0.5f, 1.f, 1.f };

static struct
{
	struct { float fovY, nearPlane, farPlane; } projection;
	struct { vec3f_t from, to, forward, right, up; } camera;
} gScene;

void d3dUpdateBuffer(ID3D11Buffer* buff, const void* data, size_t size)
{
	D3D11_MAPPED_SUBRESOURCE res = {};
	gDC->Map(buff, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	memcpy(res.pData, data, size);
	gDC->Unmap(buff, 0);
}

ID3D11Buffer* d3dMakeBuffer(UINT bindingMask, bool dynamic, size_t size, const void* data = nullptr)
{
	CHECK(size);
	ID3D11Buffer* result = NULL;
	D3D11_BUFFER_DESC desc = {};
	D3D11_SUBRESOURCE_DATA* pInitData = NULL, initData = {};
	desc.Usage = (dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = size & UINT_MAX;
	desc.BindFlags = bindingMask;
	if (dynamic)
	{
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	if (data)
	{
		initData.pSysMem = data;
		pInitData = &initData;
	}
	ENSURE(!FAILED(gDevice->CreateBuffer(&desc, pInitData, &result)));
	return result;
}

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
	d3dUpdateBuffer(gGlobalParamsBuffer, &gGlobalParams, sizeof(gGlobalParams));
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
	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_1;
	D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1 };
	D3D11CreateDeviceAndSwapChain(
		NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 
		D3D11_CREATE_DEVICE_DEBUG
		,levels, 2, D3D11_SDK_VERSION, &scd, &gSwapchain, &gDevice, &level, &gDC);
	RECT windowRect;
	GetClientRect(hWnd, &windowRect);
	int w = (windowRect.right - windowRect.left) & INT_MAX;
	int h = (windowRect.bottom - windowRect.top) & INT_MAX;
	gGlobalParamsBuffer = d3dMakeBuffer(D3D11_BIND_CONSTANT_BUFFER, true, sizeof(gGlobalParams));
	d3dResizeFrame(w, h);
	gDC->VSSetConstantBuffers(0, 1, &gGlobalParamsBuffer);
	gQuadMeshVertexBuffer = d3dMakeBuffer(D3D11_BIND_VERTEX_BUFFER, false, sizeof(gQuadMeshData), gQuadMeshData);
	ID3D10Blob *vsBlob = NULL, *psBlob = NULL;
	ENSURE(D3DReadFileToBlob(L"default_vs.cso", &vsBlob) == S_OK);
	ENSURE(D3DReadFileToBlob(L"default_ps.cso", &psBlob) == S_OK);
	gDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &gVertexShader);
	gDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &gPixelShader);
	gDC->VSSetShader(gVertexShader, 0, 0);
	gDC->PSSetShader(gPixelShader, 0, 0);
	const D3D11_INPUT_ELEMENT_DESC vertexFormat[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0 , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12 , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24 , D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	gDevice->CreateInputLayout(vertexFormat, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &gQuadVertexLayout);
	gDC->IASetInputLayout(gQuadVertexLayout);
}

static void d3dRenderFrame()
{
	gDC->ClearRenderTargetView(gBackBuffer, gClearColor);
	gDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT stride = sizeof(vertex_t), offset = 0;
	gDC->IASetVertexBuffers(0, 1, &gQuadMeshVertexBuffer, &stride, &offset);
	gDC->Draw(6, 0);
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

typedef enum
{
	OBJ_COMMENT,
	OBJ_VERTEX_POS,
	OBJ_VERTEX_NORM,
	OBJ_VERTEX_TEXCOORD,
	OBJ_FACE,
	OBJ_OBJECT
} objToken_t;

objToken_t objParseToken(const char* str, const char** valStr)
{
    objToken_t token = OBJ_COMMENT;
    switch (*str++)
    {
    case 'o':
    case 'O':
        token = OBJ_OBJECT;
        break;
    case 'v':
    case 'V':
        switch (*str++)
        {
        case 'n':
        case 'N':
            token = OBJ_VERTEX_NORM;
            break;
        case 't':
        case 'T':
            token = OBJ_VERTEX_TEXCOORD;
            break;
        default :
            token = OBJ_VERTEX_POS;
            break;
        }
        break;
    case 'f':
    case 'F':
        token = OBJ_FACE;
        break;
    default:
        break;
    }
    *valStr = str;
    return token;
}

void objParseVector2f(const char* str, vec2f_t out)
{
	char* separator = NULL;
	float* currentVal = &out[0];
	const char* readPtr = str;
	out[0] = strtof(readPtr, &separator);
	readPtr = separator;
	out[1] = strtof(readPtr, &separator);
}

void objParseVector3f(const char* str, vec3f_t out)
{
	char* separator = NULL;
	float* currentVal = &out[0];
	const char* readPtr = str;
	out[0] = strtof(readPtr, &separator);
	readPtr = separator;
	out[1] = strtof(readPtr, &separator);
	readPtr = separator;
	out[2] = strtof(readPtr, &separator);
}

const char* objParseVertex(const char* str, vec3i_t out)
{
	char* separator = NULL;
	int* currentVal = &out[0];
	const char* readPtr = str;
	out[2] = out[1] = out[0] = -1;
	while (str)
	{
		*currentVal = (strtol(readPtr, &separator, 10) & INT_MAX) - 1;
		if (separator == readPtr)
		{
			*currentVal = -1;
		}
		else if (*separator != '/')
		{
			break;
		}
		readPtr = separator + 1;
		currentVal += 1;
	}
	return separator;
}

FORCE_INLINE
void objParseTriangle(const char* str, vec3i_t vertexA, vec3i_t vertexB, vec3i_t vertexC)
{
	objParseVertex(objParseVertex(objParseVertex(str, vertexA), vertexB), vertexC);
}

int CALLBACK WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
	vec3i_t a = { 0, 0, 0 };
	vec3i_t b = { 0, 0, 0 };
	vec3i_t c = { 0, 0, 0 };
	const char* face = "4 5 6";
	objParseVertex(objParseVertex(objParseVertex(face, a), b), c);
	face = " 9//1 8//2 7//3";
	objParseVertex(objParseVertex(objParseVertex(face, a), b), c);

	vec3f_t p = {0.f, 0.f, 0.f};
	objParseVector3f(" 5 7 8", p);
	objParseVector3f(" .5 7 0.8", p);

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
			vec3f_t camera {0.f, 0.f,-5.f};
			vec3f_t origin {0.f, 0.f, 0.f};
			vec3f_t up     {0.f, 1.f, 0.f};
			glfwGetWindowSize(window, &width, &height);
			mathMat4fIdentity(gGlobalParams.view);
			mathMat4LookAt(gGlobalParams.view, camera, origin, up);
			mathMat4fPerspective(gGlobalParams.projection, mathDeg2Rad(60.f), width / ((float)height), 0.01f, 1000.f);
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
