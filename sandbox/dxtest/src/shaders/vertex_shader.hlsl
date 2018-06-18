struct VSOut
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord0;
};

VSOut vsMain( uint id : SV_VertexID )
{
	VSOut stageOut;
	uint2 tokens = uint2((id & 1), (id >> 1));
	stageOut.texCoord = float2(tokens) * 2.0;
	float2 pos = stageOut.texCoord * float2(2.0, -2.0);
	stageOut.position = float4(float2(-1.0, 1.0) + pos, 0.0, 1.0);
	return stageOut;
}