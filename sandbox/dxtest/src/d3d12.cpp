#include "d3d12.hpp"

#include <cstring>

static D3D12DeviceContext sD3D12ContextInst;
D3D12DeviceContext* const gD3D12Context = &sD3D12ContextInst;

void D3D12DeviceContext::CreateVertexArray(const VertexData& vData, D3D12VertexArray& vao)
{
	D3D12_RESOURCE_DESC decsr = {};
	descr.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	descr.Height = 1;
	descr.DepthOrArraySize = 1;
	descr.MipLevels = 1;
	descr.Format = DXGI_FORMAT_UNKNOWN;
	descr.SampleDesc.Count = 1;
	descr.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	for (UINT i = 0; i < vData.vboCount; i++)
	{
		void* mappedPtr = NULL;
		ID3D12Resource* result = NULL;
		descr.Width = vData.vboSize[i];
		assert(!FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &descr, initState, NULL, IID_PPV_ARGS(&result))));
		vao.vertexBuffers[i].resource = result;
		vao.vertexBuffers[i].size = vData.vboSize[i];
		vao.vertexBuffers[i].stride = vData.vboStride[i];
		assert(!FAILED(result->Map(0, NULL, mappedPtr)));
		memcpy(mappedPtr, vData.vboData[i], vData.vboSize[i]);
		assert(!FAILED(result->Unmap(0, NULL)));
	}
	if (vData.iboData && vData.iboSize && vData.iboType)
	{
		void* mappedPtr = NULL;
		descr.Width = vData.iboSize;
		initState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		assert(!FAILED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &descr, initState, NULL, IID_PPV_ARGS(&vao.indexBuffer))));
		vao.indexBufferSize = vData.iboSize;
		assert(!FAILED(vao.indexBuffer->Map(0, NULL, mappedPtr)));
		memcpy(mappedPtr, vData.iboData, vData.iboSize);
		assert(!FAILED(vao.indexBuffer->Unmap(0, NULL)));
		switch (vData.indexSize)
		{
		case 1: vao.indexFormat = DXGI_FORMAT_R8_UINT; break;
		case 2: vao.indexFormat = DXGI_FORMAT_R16_UINT; break;
		case 4: vao.indexFormat = DXGI_FORMAT_R32_UINT; break;
		default: assert(NULL); break;
		}
		vao.drawCount = vData.iboSize / vData.indexSize;
	}
	else
	{
		vao.drawCount = vData.numVertices;
	}
	vao.numVertexBuffers = vData.vboCount;
	vao.topology = vData.topology;
}

void D3D12DeviceContext::Draw(const D3D12VertexArray& vao)
{
	D3D12_VERTEX_BUFFER_VIEW views[MAX_VAO_BUFFERS];
	for (UINT i = 0; i < vao.numVertexBuffers; i++)
	{
		const auto& vBuffer = vao.vertexBuffers[i];
		views[i].BufferLocation = vBuffer.resource->GetGPUVirtualAddress();
		views[i].StrideInBytes = vBuffer.stride;
		views[i].SizeInBytes = vBuffer.size;
	}
	commandList->IASetPrimitiveTopology(vao.topology);
	commandList->IASetVertexBuffers(0, vao.numVertexBuffers, views);
	if (vao.indexBuffer)
	{
		D3D12_INDEX_BUFFER_VIEW view;
		view.BufferLocation = vao.indexBuffer->GetGPUVirtualAddress();
		view.Format = vao.indexFormat;
		view.SizeInBytes = vao.indexBufferSize;
		commandList->IASetIndexBuffer(&view);
		commandList->DrawIndexedInstanced(vao.drawCount, 1, 0, 0, 0);
	}
	else
	{
		commandList->DrawInstanced(vao.drawCount, 1, 0, 0);
	}
}
