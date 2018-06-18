struct PSIn
{
	float4 position : SV_Position;
	float2 texCoord : TexCoord0;
};

float4 psMain(PSIn stageIn) : SV_Target0
{
	return float4(0.0, 0.5 + stageIn.texCoord.y * 0.5, 1.0, 1.0);
}