#pragma once

#include <d3d12.h>

#define MAX_VAO_BUFFERS 8U

#define RELEASE(p) if((p)){(p)->Release();(p) = NULL;}

struct VertexData
{

	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	UINT numVertices = 0;

	UINT vboCount = 0;

	void* vboData[MAX_VAO_BUFFERS];
	
	UINT vboStride[MAX_VAO_BUFFERS];
	
	UINT vboSize[MAX_VAO_BUFFERS];

	void* iboData = NULL;

	UINT indexSize = 1;

	UINT iboSize = 0;

};

struct D3D12VertexArray
{

	struct
	{
		ID3D12Resource* resource;
		UINT size;
		UINT stride;
	} vertexBuffers[MAX_VAO_BUFFERS];

	UINT numVertexBuffers;

	ID3D12resource* indexBuffer;

	DXGI_FORMAT indexFormat;

	UINT indexBufferSize;

	D3D_PRIMITIVE_TOPOLOGY topology;

	UINT drawCount;

};

class D3D12DeviceContext;

extern D3D12DeviceContext* const gD3D12Context;

class D3D12DeviceContext
{

public:

	void CreateVertexArray(const VertexData& vData, D3D12VertexArray& vao);

	void Draw(const D3D12VertexArray& vao);

private:

	ID3D12CommandList* commandList;

};
