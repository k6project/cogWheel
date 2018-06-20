struct PSIn
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord0;
};

float4 psMain(PSIn stageIn) : SV_Target0
{
	float3 rgb = float3(1.0 - dot(stageIn.texCoord, stageIn.texCoord), stageIn.texCoord);
	return float4(rgb, 1.0);
}