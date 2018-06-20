cbuffer globalParams : register(b0)
{
	float4x4 gProjection;
	float4x4 gView;
}

cbuffer objectParams : register(b1)
{
	struct 
	{
		float4x4 model;
		float4x4 normal;
	} objects[256];
}

struct VSOut
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord0;
};

VSOut vsMain( uint id : SV_VertexID )
{
	VSOut stageOut;
	uint2 tokens = uint2((id & 1), (id >> 1));
	stageOut.texCoord = float2(tokens);
	stageOut.position = float4(stageOut.texCoord * float2(1.0, -1.0) * 0.5, 0.f, 1.0);
#ifdef FSRECT
	stageOut.texCoord = float2(tokens) * 2.0;
	float2 pos = stageOut.texCoord * float2(2.0, -2.0);
	stageOut.position = float4(float2(-1.0, 1.0) + pos, 0.0, 1.0);
#else
	//stageOut.position.x += 0.5 * (1.0 - float(tokens.x + tokens.y));
	//stageOut.position.y -= float(tokens.x);
	//stageOut.position.xy += float2(-0.5, 0.5);
	stageOut.position = mul(gProjection, mul(gView, stageOut.position));
#endif
	return stageOut;
}