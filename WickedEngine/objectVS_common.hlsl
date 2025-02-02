#include "objectHF.hlsli"



PixelInputType main(Input_Object_ALL input)
{
	PixelInputType Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	float4x4 WORLDPREV = MakeWorldMatrixFromInstance(input.instPrev);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(WORLD, surface.position);
	surface.prevPos = mul(WORLDPREV, surface.prevPos);
	surface.normal = normalize(mul((float3x3)WORLD, surface.normal));

	Out.clip = dot(surface.position, g_xClipPlane);

	Out.pos = Out.pos2D = mul(g_xCamera_VP, surface.position);
	Out.pos2DPrev = mul(g_xFrame_MainCamera_PrevVP, surface.prevPos);
	Out.pos3D = surface.position.xyz;
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.atl = surface.atlas;
	Out.nor = surface.normal;
	Out.nor2D = mul((float3x3)g_xCamera_View, Out.nor.xyz).xy;

	Out.ReflectionMapSamplingPos = mul(g_xFrame_MainCamera_ReflVP, surface.position);

	return Out;
}