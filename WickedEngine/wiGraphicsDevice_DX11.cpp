#include "wiGraphicsDevice_DX11.h"
#include "wiHelper.h"
#include "ResourceMapping.h"
#include "wiBackLog.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#include <sstream>
#include <algorithm>

using namespace std;

namespace wiGraphics
{
// Engine -> Native converters

constexpr uint32_t _ParseBindFlags(uint32_t value)
{
	uint32_t _flag = 0;

	if (value & BIND_VERTEX_BUFFER)
		_flag |= D3D11_BIND_VERTEX_BUFFER;
	if (value & BIND_INDEX_BUFFER)
		_flag |= D3D11_BIND_INDEX_BUFFER;
	if (value & BIND_CONSTANT_BUFFER)
		_flag |= D3D11_BIND_CONSTANT_BUFFER;
	if (value & BIND_SHADER_RESOURCE)
		_flag |= D3D11_BIND_SHADER_RESOURCE;
	if (value & BIND_STREAM_OUTPUT)
		_flag |= D3D11_BIND_STREAM_OUTPUT;
	if (value & BIND_RENDER_TARGET)
		_flag |= D3D11_BIND_RENDER_TARGET;
	if (value & BIND_DEPTH_STENCIL)
		_flag |= D3D11_BIND_DEPTH_STENCIL;
	if (value & BIND_UNORDERED_ACCESS)
		_flag |= D3D11_BIND_UNORDERED_ACCESS;

	return _flag;
}
constexpr uint32_t _ParseCPUAccessFlags(uint32_t value)
{
	uint32_t _flag = 0;

	if (value & CPU_ACCESS_WRITE)
		_flag |= D3D11_CPU_ACCESS_WRITE;
	if (value & CPU_ACCESS_READ)
		_flag |= D3D11_CPU_ACCESS_READ;

	return _flag;
}
constexpr uint32_t _ParseResourceMiscFlags(uint32_t value)
{
	uint32_t _flag = 0;

	if (value & RESOURCE_MISC_SHARED)
		_flag |= D3D11_RESOURCE_MISC_SHARED;
	if (value & RESOURCE_MISC_TEXTURECUBE)
		_flag |= D3D11_RESOURCE_MISC_TEXTURECUBE;
	if (value & RESOURCE_MISC_INDIRECT_ARGS)
		_flag |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	if (value & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		_flag |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	if (value & RESOURCE_MISC_BUFFER_STRUCTURED)
		_flag |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	if (value & RESOURCE_MISC_TILED)
		_flag |= D3D11_RESOURCE_MISC_TILED;

	return _flag;
}
constexpr uint32_t _ParseColorWriteMask(uint32_t value)
{
	uint32_t _flag = 0;

	if (value == D3D11_COLOR_WRITE_ENABLE_ALL)
	{
		return D3D11_COLOR_WRITE_ENABLE_ALL;
	}
	else
	{
		if (value & COLOR_WRITE_ENABLE_RED)
			_flag |= D3D11_COLOR_WRITE_ENABLE_RED;
		if (value & COLOR_WRITE_ENABLE_GREEN)
			_flag |= D3D11_COLOR_WRITE_ENABLE_GREEN;
		if (value & COLOR_WRITE_ENABLE_BLUE)
			_flag |= D3D11_COLOR_WRITE_ENABLE_BLUE;
		if (value & COLOR_WRITE_ENABLE_ALPHA)
			_flag |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	}

	return _flag;
}

constexpr D3D11_FILTER _ConvertFilter(FILTER value)
{
	switch (value)
	{
	case FILTER_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_MIN_MAG_MIP_POINT;
		break;
	case FILTER_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_ANISOTROPIC:
		return D3D11_FILTER_ANISOTROPIC;
		break;
	case FILTER_COMPARISON_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_COMPARISON_ANISOTROPIC:
		return D3D11_FILTER_COMPARISON_ANISOTROPIC;
		break;
	case FILTER_MINIMUM_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_MINIMUM_ANISOTROPIC:
		return D3D11_FILTER_MINIMUM_ANISOTROPIC;
		break;
	case FILTER_MAXIMUM_MIN_MAG_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
		break;
	case FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
		return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
		break;
	case FILTER_MAXIMUM_ANISOTROPIC:
		return D3D11_FILTER_MAXIMUM_ANISOTROPIC;
		break;
	default:
		break;
	}
	return D3D11_FILTER_MIN_MAG_MIP_POINT;
}
constexpr D3D11_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
{
	switch (value)
	{
	case TEXTURE_ADDRESS_WRAP:
		return D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	case TEXTURE_ADDRESS_MIRROR:
		return D3D11_TEXTURE_ADDRESS_MIRROR;
		break;
	case TEXTURE_ADDRESS_CLAMP:
		return D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case TEXTURE_ADDRESS_BORDER:
		return D3D11_TEXTURE_ADDRESS_BORDER;
		break;
	case TEXTURE_ADDRESS_MIRROR_ONCE:
		return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		break;
	default:
		break;
	}
	return D3D11_TEXTURE_ADDRESS_WRAP;
}
constexpr D3D11_COMPARISON_FUNC _ConvertComparisonFunc(COMPARISON_FUNC value)
{
	switch (value)
	{
	case COMPARISON_NEVER:
		return D3D11_COMPARISON_NEVER;
		break;
	case COMPARISON_LESS:
		return D3D11_COMPARISON_LESS;
		break;
	case COMPARISON_EQUAL:
		return D3D11_COMPARISON_EQUAL;
		break;
	case COMPARISON_LESS_EQUAL:
		return D3D11_COMPARISON_LESS_EQUAL;
		break;
	case COMPARISON_GREATER:
		return D3D11_COMPARISON_GREATER;
		break;
	case COMPARISON_NOT_EQUAL:
		return D3D11_COMPARISON_NOT_EQUAL;
		break;
	case COMPARISON_GREATER_EQUAL:
		return D3D11_COMPARISON_GREATER_EQUAL;
		break;
	case COMPARISON_ALWAYS:
		return D3D11_COMPARISON_ALWAYS;
		break;
	default:
		break;
	}
	return D3D11_COMPARISON_NEVER;
}
constexpr D3D11_FILL_MODE _ConvertFillMode(FILL_MODE value)
{
	switch (value)
	{
	case FILL_WIREFRAME:
		return D3D11_FILL_WIREFRAME;
		break;
	case FILL_SOLID:
		return D3D11_FILL_SOLID;
		break;
	default:
		break;
	}
	return D3D11_FILL_WIREFRAME;
}
constexpr D3D11_CULL_MODE _ConvertCullMode(CULL_MODE value)
{
	switch (value)
	{
	case CULL_NONE:
		return D3D11_CULL_NONE;
		break;
	case CULL_FRONT:
		return D3D11_CULL_FRONT;
		break;
	case CULL_BACK:
		return D3D11_CULL_BACK;
		break;
	default:
		break;
	}
	return D3D11_CULL_NONE;
}
constexpr D3D11_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DEPTH_WRITE_MASK value)
{
	switch (value)
	{
	case DEPTH_WRITE_MASK_ZERO:
		return D3D11_DEPTH_WRITE_MASK_ZERO;
		break;
	case DEPTH_WRITE_MASK_ALL:
		return D3D11_DEPTH_WRITE_MASK_ALL;
		break;
	default:
		break;
	}
	return D3D11_DEPTH_WRITE_MASK_ZERO;
}
constexpr D3D11_STENCIL_OP _ConvertStencilOp(STENCIL_OP value)
{
	switch (value)
	{
	case STENCIL_OP_KEEP:
		return D3D11_STENCIL_OP_KEEP;
		break;
	case STENCIL_OP_ZERO:
		return D3D11_STENCIL_OP_ZERO;
		break;
	case STENCIL_OP_REPLACE:
		return D3D11_STENCIL_OP_REPLACE;
		break;
	case STENCIL_OP_INCR_SAT:
		return D3D11_STENCIL_OP_INCR_SAT;
		break;
	case STENCIL_OP_DECR_SAT:
		return D3D11_STENCIL_OP_DECR_SAT;
		break;
	case STENCIL_OP_INVERT:
		return D3D11_STENCIL_OP_INVERT;
		break;
	case STENCIL_OP_INCR:
		return D3D11_STENCIL_OP_INCR;
		break;
	case STENCIL_OP_DECR:
		return D3D11_STENCIL_OP_DECR;
		break;
	default:
		break;
	}
	return D3D11_STENCIL_OP_KEEP;
}
constexpr D3D11_BLEND _ConvertBlend(BLEND value)
{
	switch (value)
	{
	case BLEND_ZERO:
		return D3D11_BLEND_ZERO;
		break;
	case BLEND_ONE:
		return D3D11_BLEND_ONE;
		break;
	case BLEND_SRC_COLOR:
		return D3D11_BLEND_SRC_COLOR;
		break;
	case BLEND_INV_SRC_COLOR:
		return D3D11_BLEND_INV_SRC_COLOR;
		break;
	case BLEND_SRC_ALPHA:
		return D3D11_BLEND_SRC_ALPHA;
		break;
	case BLEND_INV_SRC_ALPHA:
		return D3D11_BLEND_INV_SRC_ALPHA;
		break;
	case BLEND_DEST_ALPHA:
		return D3D11_BLEND_DEST_ALPHA;
		break;
	case BLEND_INV_DEST_ALPHA:
		return D3D11_BLEND_INV_DEST_ALPHA;
		break;
	case BLEND_DEST_COLOR:
		return D3D11_BLEND_DEST_COLOR;
		break;
	case BLEND_INV_DEST_COLOR:
		return D3D11_BLEND_INV_DEST_COLOR;
		break;
	case BLEND_SRC_ALPHA_SAT:
		return D3D11_BLEND_SRC_ALPHA_SAT;
		break;
	case BLEND_BLEND_FACTOR:
		return D3D11_BLEND_BLEND_FACTOR;
		break;
	case BLEND_INV_BLEND_FACTOR:
		return D3D11_BLEND_INV_BLEND_FACTOR;
		break;
	case BLEND_SRC1_COLOR:
		return D3D11_BLEND_SRC1_COLOR;
		break;
	case BLEND_INV_SRC1_COLOR:
		return D3D11_BLEND_INV_SRC1_COLOR;
		break;
	case BLEND_SRC1_ALPHA:
		return D3D11_BLEND_SRC1_ALPHA;
		break;
	case BLEND_INV_SRC1_ALPHA:
		return D3D11_BLEND_INV_SRC1_ALPHA;
		break;
	default:
		break;
	}
	return D3D11_BLEND_ZERO;
}
constexpr D3D11_BLEND_OP _ConvertBlendOp(BLEND_OP value)
{
	switch (value)
	{
	case BLEND_OP_ADD:
		return D3D11_BLEND_OP_ADD;
		break;
	case BLEND_OP_SUBTRACT:
		return D3D11_BLEND_OP_SUBTRACT;
		break;
	case BLEND_OP_REV_SUBTRACT:
		return D3D11_BLEND_OP_REV_SUBTRACT;
		break;
	case BLEND_OP_MIN:
		return D3D11_BLEND_OP_MIN;
		break;
	case BLEND_OP_MAX:
		return D3D11_BLEND_OP_MAX;
		break;
	default:
		break;
	}
	return D3D11_BLEND_OP_ADD;
}
constexpr D3D11_USAGE _ConvertUsage(USAGE value)
{
	switch (value)
	{
	case USAGE_DEFAULT:
		return D3D11_USAGE_DEFAULT;
		break;
	case USAGE_IMMUTABLE:
		return D3D11_USAGE_IMMUTABLE;
		break;
	case USAGE_DYNAMIC:
		return D3D11_USAGE_DYNAMIC;
		break;
	case USAGE_STAGING:
		return D3D11_USAGE_STAGING;
		break;
	default:
		break;
	}
	return D3D11_USAGE_DEFAULT;
}
constexpr D3D11_INPUT_CLASSIFICATION _ConvertInputClassification(INPUT_CLASSIFICATION value)
{
	switch (value)
	{
	case INPUT_PER_VERTEX_DATA:
		return D3D11_INPUT_PER_VERTEX_DATA;
		break;
	case INPUT_PER_INSTANCE_DATA:
		return D3D11_INPUT_PER_INSTANCE_DATA;
		break;
	default:
		break;
	}
	return D3D11_INPUT_PER_VERTEX_DATA;
}
constexpr DXGI_FORMAT _ConvertFormat(FORMAT value)
{
	switch (value)
	{
	case FORMAT_UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
		break;
	case FORMAT_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case FORMAT_R32G32B32A32_UINT:
		return DXGI_FORMAT_R32G32B32A32_UINT;
		break;
	case FORMAT_R32G32B32A32_SINT:
		return DXGI_FORMAT_R32G32B32A32_SINT;
		break;
	case FORMAT_R32G32B32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case FORMAT_R32G32B32_UINT:
		return DXGI_FORMAT_R32G32B32_UINT;
		break;
	case FORMAT_R32G32B32_SINT:
		return DXGI_FORMAT_R32G32B32_SINT;
		break;
	case FORMAT_R16G16B16A16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case FORMAT_R16G16B16A16_UNORM:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
		break;
	case FORMAT_R16G16B16A16_UINT:
		return DXGI_FORMAT_R16G16B16A16_UINT;
		break;
	case FORMAT_R16G16B16A16_SNORM:
		return DXGI_FORMAT_R16G16B16A16_SNORM;
		break;
	case FORMAT_R16G16B16A16_SINT:
		return DXGI_FORMAT_R16G16B16A16_SINT;
		break;
	case FORMAT_R32G32_FLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
		break;
	case FORMAT_R32G32_UINT:
		return DXGI_FORMAT_R32G32_UINT;
		break;
	case FORMAT_R32G32_SINT:
		return DXGI_FORMAT_R32G32_SINT;
		break;
	case FORMAT_R32G8X24_TYPELESS:
		return DXGI_FORMAT_R32G8X24_TYPELESS;
		break;
	case FORMAT_D32_FLOAT_S8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		break;
	case FORMAT_R10G10B10A2_UNORM:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
		break;
	case FORMAT_R10G10B10A2_UINT:
		return DXGI_FORMAT_R10G10B10A2_UINT;
		break;
	case FORMAT_R11G11B10_FLOAT:
		return DXGI_FORMAT_R11G11B10_FLOAT;
		break;
	case FORMAT_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case FORMAT_R8G8B8A8_UINT:
		return DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	case FORMAT_R8G8B8A8_SNORM:
		return DXGI_FORMAT_R8G8B8A8_SNORM;
		break;
	case FORMAT_R8G8B8A8_SINT:
		return DXGI_FORMAT_R8G8B8A8_SINT;
		break;
	case FORMAT_R16G16_FLOAT:
		return DXGI_FORMAT_R16G16_FLOAT;
		break;
	case FORMAT_R16G16_UNORM:
		return DXGI_FORMAT_R16G16_UNORM;
		break;
	case FORMAT_R16G16_UINT:
		return DXGI_FORMAT_R16G16_UINT;
		break;
	case FORMAT_R16G16_SNORM:
		return DXGI_FORMAT_R16G16_SNORM;
		break;
	case FORMAT_R16G16_SINT:
		return DXGI_FORMAT_R16G16_SINT;
		break;
	case FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_R32_TYPELESS;
		break;
	case FORMAT_D32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;
		break;
	case FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
		break;
	case FORMAT_R32_UINT:
		return DXGI_FORMAT_R32_UINT;
		break;
	case FORMAT_R32_SINT:
		return DXGI_FORMAT_R32_SINT;
		break;
	case FORMAT_R24G8_TYPELESS:
		return DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case FORMAT_D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	case FORMAT_R8G8_UNORM:
		return DXGI_FORMAT_R8G8_UNORM;
		break;
	case FORMAT_R8G8_UINT:
		return DXGI_FORMAT_R8G8_UINT;
		break;
	case FORMAT_R8G8_SNORM:
		return DXGI_FORMAT_R8G8_SNORM;
		break;
	case FORMAT_R8G8_SINT:
		return DXGI_FORMAT_R8G8_SINT;
		break;
	case FORMAT_R16_TYPELESS:
		return DXGI_FORMAT_R16_TYPELESS;
		break;
	case FORMAT_R16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
		break;
	case FORMAT_D16_UNORM:
		return DXGI_FORMAT_D16_UNORM;
		break;
	case FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;
		break;
	case FORMAT_R16_UINT:
		return DXGI_FORMAT_R16_UINT;
		break;
	case FORMAT_R16_SNORM:
		return DXGI_FORMAT_R16_SNORM;
		break;
	case FORMAT_R16_SINT:
		return DXGI_FORMAT_R16_SINT;
		break;
	case FORMAT_R8_UNORM:
		return DXGI_FORMAT_R8_UNORM;
		break;
	case FORMAT_R8_UINT:
		return DXGI_FORMAT_R8_UINT;
		break;
	case FORMAT_R8_SNORM:
		return DXGI_FORMAT_R8_SNORM;
		break;
	case FORMAT_R8_SINT:
		return DXGI_FORMAT_R8_SINT;
		break;
	case FORMAT_BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM;
		break;
	case FORMAT_BC1_UNORM_SRGB:
		return DXGI_FORMAT_BC1_UNORM_SRGB;
		break;
	case FORMAT_BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM;
		break;
	case FORMAT_BC2_UNORM_SRGB:
		return DXGI_FORMAT_BC2_UNORM_SRGB;
		break;
	case FORMAT_BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM;
		break;
	case FORMAT_BC3_UNORM_SRGB:
		return DXGI_FORMAT_BC3_UNORM_SRGB;
		break;
	case FORMAT_BC4_UNORM:
		return DXGI_FORMAT_BC4_UNORM;
		break;
	case FORMAT_BC4_SNORM:
		return DXGI_FORMAT_BC4_SNORM;
		break;
	case FORMAT_BC5_UNORM:
		return DXGI_FORMAT_BC5_UNORM;
		break;
	case FORMAT_BC5_SNORM:
		return DXGI_FORMAT_BC5_SNORM;
		break;
	case FORMAT_B8G8R8A8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	case FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case FORMAT_BC6H_UF16:
		return DXGI_FORMAT_BC6H_UF16;
		break;
	case FORMAT_BC6H_SF16:
		return DXGI_FORMAT_BC6H_SF16;
		break;
	case FORMAT_BC7_UNORM:
		return DXGI_FORMAT_BC7_UNORM;
		break;
	case FORMAT_BC7_UNORM_SRGB:
		return DXGI_FORMAT_BC7_UNORM_SRGB;
		break;
	}
	return DXGI_FORMAT_UNKNOWN;
}

inline D3D11_TEXTURE1D_DESC _ConvertTextureDesc1D(const TextureDesc* pDesc)
{
	D3D11_TEXTURE1D_DESC desc;
	desc.Width = pDesc->Width;
	desc.MipLevels = pDesc->MipLevels;
	desc.ArraySize = pDesc->ArraySize;
	desc.Format = _ConvertFormat(pDesc->Format);
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

	return desc;
}
inline D3D11_TEXTURE2D_DESC _ConvertTextureDesc2D(const TextureDesc* pDesc)
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = pDesc->Width;
	desc.Height = pDesc->Height;
	desc.MipLevels = pDesc->MipLevels;
	desc.ArraySize = pDesc->ArraySize;
	desc.Format = _ConvertFormat(pDesc->Format);
	desc.SampleDesc.Count = pDesc->SampleCount;
	desc.SampleDesc.Quality = 0;
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

	return desc;
}
inline D3D11_TEXTURE3D_DESC _ConvertTextureDesc3D(const TextureDesc* pDesc)
{
	D3D11_TEXTURE3D_DESC desc;
	desc.Width = pDesc->Width;
	desc.Height = pDesc->Height;
	desc.Depth = pDesc->Depth;
	desc.MipLevels = pDesc->MipLevels;
	desc.Format = _ConvertFormat(pDesc->Format);
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);

	return desc;
}
inline D3D11_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
{
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = pInitialData.pSysMem;
	data.SysMemPitch = pInitialData.SysMemPitch;
	data.SysMemSlicePitch = pInitialData.SysMemSlicePitch;

	return data;
}


// Native -> Engine converters

constexpr uint32_t _ParseBindFlags_Inv(uint32_t value)
{
	uint32_t _flag = 0;

	if (value & D3D11_BIND_VERTEX_BUFFER)
		_flag |= BIND_VERTEX_BUFFER;
	if (value & D3D11_BIND_INDEX_BUFFER)
		_flag |= BIND_INDEX_BUFFER;
	if (value & D3D11_BIND_CONSTANT_BUFFER)
		_flag |= BIND_CONSTANT_BUFFER;
	if (value & D3D11_BIND_SHADER_RESOURCE)
		_flag |= BIND_SHADER_RESOURCE;
	if (value & D3D11_BIND_STREAM_OUTPUT)
		_flag |= BIND_STREAM_OUTPUT;
	if (value & D3D11_BIND_RENDER_TARGET)
		_flag |= BIND_RENDER_TARGET;
	if (value & D3D11_BIND_DEPTH_STENCIL)
		_flag |= BIND_DEPTH_STENCIL;
	if (value & D3D11_BIND_UNORDERED_ACCESS)
		_flag |= BIND_UNORDERED_ACCESS;

	return _flag;
}
constexpr uint32_t _ParseCPUAccessFlags_Inv(uint32_t value)
{
	uint32_t _flag = 0;

	if (value & D3D11_CPU_ACCESS_WRITE)
		_flag |= CPU_ACCESS_WRITE;
	if (value & D3D11_CPU_ACCESS_READ)
		_flag |= CPU_ACCESS_READ;

	return _flag;
}
constexpr uint32_t _ParseResourceMiscFlags_Inv(uint32_t value)
{
	uint32_t _flag = 0;

	if (value & D3D11_RESOURCE_MISC_SHARED)
		_flag |= RESOURCE_MISC_SHARED;
	if (value & D3D11_RESOURCE_MISC_TEXTURECUBE)
		_flag |= RESOURCE_MISC_TEXTURECUBE;
	if (value & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
		_flag |= RESOURCE_MISC_INDIRECT_ARGS;
	if (value & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
		_flag |= RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	if (value & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
		_flag |= RESOURCE_MISC_BUFFER_STRUCTURED;
	if (value & D3D11_RESOURCE_MISC_TILED)
		_flag |= RESOURCE_MISC_TILED;

	return _flag;
}

constexpr FORMAT _ConvertFormat_Inv(DXGI_FORMAT value)
{
	switch (value)
	{
	case DXGI_FORMAT_UNKNOWN:
		return FORMAT_UNKNOWN;
		break;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return FORMAT_R32G32B32A32_FLOAT;
		break;
	case DXGI_FORMAT_R32G32B32A32_UINT:
		return FORMAT_R32G32B32A32_UINT;
		break;
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return FORMAT_R32G32B32A32_SINT;
		break;
	case DXGI_FORMAT_R32G32B32_FLOAT:
		return FORMAT_R32G32B32_FLOAT;
		break;
	case DXGI_FORMAT_R32G32B32_UINT:
		return FORMAT_R32G32B32_UINT;
		break;
	case DXGI_FORMAT_R32G32B32_SINT:
		return FORMAT_R32G32B32_SINT;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return FORMAT_R16G16B16A16_FLOAT;
		break;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return FORMAT_R16G16B16A16_UNORM;
		break;
	case DXGI_FORMAT_R16G16B16A16_UINT:
		return FORMAT_R16G16B16A16_UINT;
		break;
	case DXGI_FORMAT_R16G16B16A16_SNORM:
		return FORMAT_R16G16B16A16_SNORM;
		break;
	case DXGI_FORMAT_R16G16B16A16_SINT:
		return FORMAT_R16G16B16A16_SINT;
		break;
	case DXGI_FORMAT_R32G32_FLOAT:
		return FORMAT_R32G32_FLOAT;
		break;
	case DXGI_FORMAT_R32G32_UINT:
		return FORMAT_R32G32_UINT;
		break;
	case DXGI_FORMAT_R32G32_SINT:
		return FORMAT_R32G32_SINT;
		break;
	case DXGI_FORMAT_R32G8X24_TYPELESS:
		return FORMAT_R32G8X24_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		return FORMAT_D32_FLOAT_S8X24_UINT;
		break;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return FORMAT_R10G10B10A2_UNORM;
		break;
	case DXGI_FORMAT_R10G10B10A2_UINT:
		return FORMAT_R10G10B10A2_UINT;
		break;
	case DXGI_FORMAT_R11G11B10_FLOAT:
		return FORMAT_R11G11B10_FLOAT;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return FORMAT_R8G8B8A8_UNORM;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_R8G8B8A8_UINT:
		return FORMAT_R8G8B8A8_UINT;
		break;
	case DXGI_FORMAT_R8G8B8A8_SNORM:
		return FORMAT_R8G8B8A8_SNORM;
		break;
	case DXGI_FORMAT_R8G8B8A8_SINT:
		return FORMAT_R8G8B8A8_SINT;
		break;
	case DXGI_FORMAT_R16G16_FLOAT:
		return FORMAT_R16G16_FLOAT;
		break;
	case DXGI_FORMAT_R16G16_UNORM:
		return FORMAT_R16G16_UNORM;
		break;
	case DXGI_FORMAT_R16G16_UINT:
		return FORMAT_R16G16_UINT;
		break;
	case DXGI_FORMAT_R16G16_SNORM:
		return FORMAT_R16G16_SNORM;
		break;
	case DXGI_FORMAT_R16G16_SINT:
		return FORMAT_R16G16_SINT;
		break;
	case DXGI_FORMAT_R32_TYPELESS:
		return FORMAT_R32_TYPELESS;
		break;
	case DXGI_FORMAT_D32_FLOAT:
		return FORMAT_D32_FLOAT;
		break;
	case DXGI_FORMAT_R32_FLOAT:
		return FORMAT_R32_FLOAT;
		break;
	case DXGI_FORMAT_R32_UINT:
		return FORMAT_R32_UINT;
		break;
	case DXGI_FORMAT_R32_SINT:
		return FORMAT_R32_SINT;
		break;
	case DXGI_FORMAT_R24G8_TYPELESS:
		return FORMAT_R24G8_TYPELESS;
		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return FORMAT_D24_UNORM_S8_UINT;
		break;
	case DXGI_FORMAT_R8G8_UNORM:
		return FORMAT_R8G8_UNORM;
		break;
	case DXGI_FORMAT_R8G8_UINT:
		return FORMAT_R8G8_UINT;
		break;
	case DXGI_FORMAT_R8G8_SNORM:
		return FORMAT_R8G8_SNORM;
		break;
	case DXGI_FORMAT_R8G8_SINT:
		return FORMAT_R8G8_SINT;
		break;
	case DXGI_FORMAT_R16_TYPELESS:
		return FORMAT_R16_TYPELESS;
		break;
	case DXGI_FORMAT_R16_FLOAT:
		return FORMAT_R16_FLOAT;
		break;
	case DXGI_FORMAT_D16_UNORM:
		return FORMAT_D16_UNORM;
		break;
	case DXGI_FORMAT_R16_UNORM:
		return FORMAT_R16_UNORM;
		break;
	case DXGI_FORMAT_R16_UINT:
		return FORMAT_R16_UINT;
		break;
	case DXGI_FORMAT_R16_SNORM:
		return FORMAT_R16_SNORM;
		break;
	case DXGI_FORMAT_R16_SINT:
		return FORMAT_R16_SINT;
		break;
	case DXGI_FORMAT_R8_UNORM:
		return FORMAT_R8_UNORM;
		break;
	case DXGI_FORMAT_R8_UINT:
		return FORMAT_R8_UINT;
		break;
	case DXGI_FORMAT_R8_SNORM:
		return FORMAT_R8_SNORM;
		break;
	case DXGI_FORMAT_R8_SINT:
		return FORMAT_R8_SINT;
		break;
	case DXGI_FORMAT_BC1_UNORM:
		return FORMAT_BC1_UNORM;
		break;
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return FORMAT_BC1_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC2_UNORM:
		return FORMAT_BC2_UNORM;
		break;
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		return FORMAT_BC2_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC3_UNORM:
		return FORMAT_BC3_UNORM;
		break;
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return FORMAT_BC3_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC4_UNORM:
		return FORMAT_BC4_UNORM;
		break;
	case DXGI_FORMAT_BC4_SNORM:
		return FORMAT_BC4_SNORM;
		break;
	case DXGI_FORMAT_BC5_UNORM:
		return FORMAT_BC5_UNORM;
		break;
	case DXGI_FORMAT_BC5_SNORM:
		return FORMAT_BC5_SNORM;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return FORMAT_B8G8R8A8_UNORM;
		break;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return FORMAT_B8G8R8A8_UNORM_SRGB;
		break;
	case DXGI_FORMAT_BC6H_UF16:
		return FORMAT_BC6H_UF16;
		break;
	case DXGI_FORMAT_BC6H_SF16:
		return FORMAT_BC6H_SF16;
		break;
	case DXGI_FORMAT_BC7_UNORM:
		return FORMAT_BC7_UNORM;
		break;
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return FORMAT_BC7_UNORM_SRGB;
		break;
	}
	return FORMAT_UNKNOWN;
}
constexpr USAGE _ConvertUsage_Inv(D3D11_USAGE value)
{
	switch (value)
	{
	case D3D11_USAGE_DEFAULT:
		return USAGE_DEFAULT;
		break;
	case D3D11_USAGE_IMMUTABLE:
		return USAGE_IMMUTABLE;
		break;
	case D3D11_USAGE_DYNAMIC:
		return USAGE_DYNAMIC;
		break;
	case D3D11_USAGE_STAGING:
		return USAGE_STAGING;
		break;
	default:
		break;
	}
	return USAGE_DEFAULT;
}

inline TextureDesc _ConvertTextureDesc_Inv(const D3D11_TEXTURE1D_DESC* pDesc)
{
	TextureDesc desc;
	desc.Width = pDesc->Width;
	desc.MipLevels = pDesc->MipLevels;
	desc.ArraySize = pDesc->ArraySize;
	desc.Format = _ConvertFormat_Inv(pDesc->Format);
	desc.Usage = _ConvertUsage_Inv(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags_Inv(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags_Inv(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags_Inv(pDesc->MiscFlags);

	return desc;
}
inline TextureDesc _ConvertTextureDesc_Inv(const D3D11_TEXTURE2D_DESC* pDesc)
{
	TextureDesc desc;
	desc.Width = pDesc->Width;
	desc.Height = pDesc->Height;
	desc.MipLevels = pDesc->MipLevels;
	desc.ArraySize = pDesc->ArraySize;
	desc.Format = _ConvertFormat_Inv(pDesc->Format);
	desc.SampleCount = pDesc->SampleDesc.Count;
	desc.Usage = _ConvertUsage_Inv(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags_Inv(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags_Inv(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags_Inv(pDesc->MiscFlags);

	return desc;
}
inline TextureDesc _ConvertTextureDesc_Inv(const D3D11_TEXTURE3D_DESC* pDesc)
{
	TextureDesc desc;
	desc.Width = pDesc->Width;
	desc.Height = pDesc->Height;
	desc.Depth = pDesc->Depth;
	desc.MipLevels = pDesc->MipLevels;
	desc.Format = _ConvertFormat_Inv(pDesc->Format);
	desc.Usage = _ConvertUsage_Inv(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags_Inv(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags_Inv(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags_Inv(pDesc->MiscFlags);

	return desc;
}


// Local Helpers:
const void* const __nullBlob[128] = {}; // this is initialized to nullptrs and used to unbind resources!



// Engine functions

GraphicsDevice_DX11::GraphicsDevice_DX11(wiWindowRegistration::window_type window, bool fullscreen, bool debuglayer)
{
	DEBUGDEVICE = debuglayer;
	FULLSCREEN = fullscreen;

#ifndef WINSTORE_SUPPORT
	RECT rect = RECT();
	GetClientRect(window, &rect);
	SCREENWIDTH = rect.right - rect.left;
	SCREENHEIGHT = rect.bottom - rect.top;
#else WINSTORE_SUPPORT
	SCREENWIDTH = (int)window->Bounds.Width;
	SCREENHEIGHT = (int)window->Bounds.Height;
#endif

	HRESULT hr = E_FAIL;

	for (int i = 0; i < COMMANDLIST_COUNT; i++)
	{
		stencilRef[i] = 0;
		blendFactor[i] = XMFLOAT4(1, 1, 1, 1);
	}

	uint32_t createDeviceFlags = 0;

	if (debuglayer)
	{
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	uint32_t numDriverTypes = arraysize(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	uint32_t numFeatureLevels = arraysize(featureLevels);

	for (uint32_t driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &device
			, &featureLevel, &immediateContext);

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
	{
		stringstream ss("");
		ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
		wiHelper::messageBox(ss.str(), "Error!");
		exit(1);
	}

	IDXGIDevice2 * pDXGIDevice;
	hr = device->QueryInterface(__uuidof(IDXGIDevice2), (void **)&pDXGIDevice);

	IDXGIAdapter * pDXGIAdapter;
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);

	IDXGIFactory2 * pIDXGIFactory;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), (void **)&pIDXGIFactory);


	DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
	sd.Width = SCREENWIDTH;
	sd.Height = SCREENHEIGHT;
	sd.Format = _ConvertFormat(GetBackBufferFormat());
	sd.Stereo = false;
	sd.SampleDesc.Count = 1; // Don't use multi-sampling.
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2; // Use double-buffering to minimize latency.
	sd.Flags = 0;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

#ifndef WINSTORE_SUPPORT
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	sd.Scaling = DXGI_SCALING_STRETCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
	fullscreenDesc.RefreshRate.Numerator = 60;
	fullscreenDesc.RefreshRate.Denominator = 1;
	fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // needs to be unspecified for correct fullscreen scaling!
	fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
	fullscreenDesc.Windowed = !fullscreen;
	hr = pIDXGIFactory->CreateSwapChainForHwnd(device, window, &sd, &fullscreenDesc, nullptr, &swapChain);
#else
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

	hr = pIDXGIFactory->CreateSwapChainForCoreWindow(device, reinterpret_cast<IUnknown*>(window), &sd, nullptr, &swapChain);
#endif

	if (FAILED(hr))
	{
		wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
		exit(1);
	}

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	hr = pDXGIDevice->SetMaximumFrameLatency(1);


	D3D_FEATURE_LEVEL aquiredFeatureLevel = device->GetFeatureLevel();
	TESSELLATION = ((aquiredFeatureLevel >= D3D_FEATURE_LEVEL_11_0) ? true : false);

	//D3D11_FEATURE_DATA_D3D11_OPTIONS features_0;
	//hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &features_0, sizeof(features_0));

	//D3D11_FEATURE_DATA_D3D11_OPTIONS1 features_1;
	//hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &features_1, sizeof(features_1));

	D3D11_FEATURE_DATA_D3D11_OPTIONS2 features_2;
	hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &features_2, sizeof(features_2));
	CONSERVATIVE_RASTERIZATION = features_2.ConservativeRasterizationTier >= D3D11_CONSERVATIVE_RASTERIZATION_TIER_1;
	RASTERIZER_ORDERED_VIEWS = features_2.ROVsSupported == TRUE;

	if (features_2.TypedUAVLoadAdditionalFormats)
	{
		// More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
		UAV_LOAD_FORMAT_COMMON = true;

		D3D11_FEATURE_DATA_FORMAT_SUPPORT2 FormatSupport = {};
		FormatSupport.InFormat = DXGI_FORMAT_R11G11B10_FLOAT;
		hr = device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &FormatSupport, sizeof(FormatSupport));
		if (SUCCEEDED(hr) && (FormatSupport.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
		{
			UAV_LOAD_FORMAT_R11G11B10_FLOAT = true;
		}
	}

	//D3D11_FEATURE_DATA_D3D11_OPTIONS3 features_3;
	//hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &features_3, sizeof(features_3));

	CreateBackBufferResources();


	wiBackLog::post("Created GraphicsDevice_DX11");
}
GraphicsDevice_DX11::~GraphicsDevice_DX11()
{
	SAFE_RELEASE(renderTargetView);
	SAFE_RELEASE(swapChain);

	for (int i = 0; i<COMMANDLIST_COUNT; i++) {
		SAFE_RELEASE(commandLists[i]);
		SAFE_RELEASE(deviceContexts[i]);
	}

	SAFE_RELEASE(device);
}

void GraphicsDevice_DX11::CreateBackBufferResources()
{
	SAFE_RELEASE(backBuffer);
	SAFE_RELEASE(renderTargetView);

	HRESULT hr;

	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	if (FAILED(hr)) {
		wiHelper::messageBox("BackBuffer creation Failed!", "Error!");
		exit(1);
	}

	hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
	if (FAILED(hr)) {
		wiHelper::messageBox("Main Rendertarget creation Failed!", "Error!");
		exit(1);
	}
}

void GraphicsDevice_DX11::SetResolution(int width, int height)
{
	if ((width != SCREENWIDTH || height != SCREENHEIGHT) && width > 0 && height > 0)
	{
		SCREENWIDTH = width;
		SCREENHEIGHT = height;

		SAFE_RELEASE(backBuffer);
		SAFE_RELEASE(renderTargetView);
		HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, _ConvertFormat(GetBackBufferFormat()), 0);
		assert(SUCCEEDED(hr));

		CreateBackBufferResources();

		RESOLUTIONCHANGED = true;
	}
}

Texture GraphicsDevice_DX11::GetBackBuffer()
{
	Texture result;
	result.resource = (wiCPUHandle)backBuffer;

	D3D11_TEXTURE2D_DESC desc;
	backBuffer->GetDesc(&desc);
	result.desc = _ConvertTextureDesc_Inv(&desc);

	return result;
}

bool GraphicsDevice_DX11::CreateBuffer(const GPUBufferDesc *pDesc, const SubresourceData* pInitialData, GPUBuffer *pBuffer)
{
	DestroyBuffer(pBuffer);
	DestroyResource(pBuffer);
	pBuffer->type = GPUResource::GPU_RESOURCE_TYPE::BUFFER;
	pBuffer->Register(this);

	D3D11_BUFFER_DESC desc; 
	desc.ByteWidth = pDesc->ByteWidth;
	desc.Usage = _ConvertUsage(pDesc->Usage);
	desc.BindFlags = _ParseBindFlags(pDesc->BindFlags);
	desc.CPUAccessFlags = _ParseCPUAccessFlags(pDesc->CPUAccessFlags);
	desc.MiscFlags = _ParseResourceMiscFlags(pDesc->MiscFlags);
	desc.StructureByteStride = pDesc->StructureByteStride;

	D3D11_SUBRESOURCE_DATA* data = nullptr;
	if (pInitialData != nullptr)
	{
		data = new D3D11_SUBRESOURCE_DATA[1];
		for (uint32_t slice = 0; slice < 1; ++slice)
		{
			data[slice] = _ConvertSubresourceData(pInitialData[slice]);
		}
	}

	pBuffer->desc = *pDesc;
	HRESULT hr = device->CreateBuffer(&desc, data, (ID3D11Buffer**)&pBuffer->resource);
	SAFE_DELETE_ARRAY(data);
	assert(SUCCEEDED(hr) && "GPUBuffer creation failed!");

	if (SUCCEEDED(hr))
	{
		// Create resource views if needed
		if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
		{

			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
			ZeroMemory(&srv_desc, sizeof(srv_desc));

			if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
				srv_desc.BufferEx.FirstElement = 0;
				srv_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
				srv_desc.BufferEx.NumElements = desc.ByteWidth / 4;
			}
			else if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				srv_desc.Format = DXGI_FORMAT_UNKNOWN;
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
				srv_desc.BufferEx.FirstElement = 0;
				srv_desc.BufferEx.NumElements = desc.ByteWidth / desc.StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				srv_desc.Format = _ConvertFormat(pDesc->Format);
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
			}

			hr = device->CreateShaderResourceView((ID3D11Resource*)pBuffer->resource, &srv_desc, (ID3D11ShaderResourceView**)&pBuffer->SRV);

			assert(SUCCEEDED(hr) && "ShaderResourceView of the GPUBuffer could not be created!");
		}
		if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
			ZeroMemory(&uav_desc, sizeof(uav_desc));
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				uav_desc.Format = DXGI_FORMAT_R32_TYPELESS; // Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
				uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
				uav_desc.Buffer.NumElements = desc.ByteWidth / 4;
			}
			else if (desc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				uav_desc.Format = DXGI_FORMAT_UNKNOWN;      // Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
				uav_desc.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				uav_desc.Format = _ConvertFormat(pDesc->Format);
				uav_desc.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
			}

			hr = device->CreateUnorderedAccessView((ID3D11Resource*)pBuffer->resource, &uav_desc, (ID3D11UnorderedAccessView**)&pBuffer->UAV);

			assert(SUCCEEDED(hr) && "UnorderedAccessView of the GPUBuffer could not be created!");
		}
	}

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateTexture(const TextureDesc* pDesc, const SubresourceData *pInitialData, Texture *pTexture)
{
	DestroyTexture(pTexture);
	DestroyResource(pTexture);
	pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;
	pTexture->Register(this);

	pTexture->desc = *pDesc;

	std::vector<D3D11_SUBRESOURCE_DATA> data;
	if (pInitialData != nullptr)
	{
		uint32_t dataCount = pDesc->ArraySize * std::max(1u, pDesc->MipLevels);
		data.resize(dataCount);
		for (uint32_t slice = 0; slice < dataCount; ++slice)
		{
			data[slice] = _ConvertSubresourceData(pInitialData[slice]);
		}
	}

	HRESULT hr = S_OK;

	switch (pTexture->desc.type)
	{
	case TextureDesc::TEXTURE_1D:
	{
		D3D11_TEXTURE1D_DESC desc = _ConvertTextureDesc1D(&pTexture->desc);
		hr = device->CreateTexture1D(&desc, data.data(), (ID3D11Texture1D**)&pTexture->resource);
	}
	break;
	case TextureDesc::TEXTURE_2D:
	{
		D3D11_TEXTURE2D_DESC desc = _ConvertTextureDesc2D(&pTexture->desc);
		hr = device->CreateTexture2D(&desc, data.data(), (ID3D11Texture2D**)&pTexture->resource);
	}
	break;
	case TextureDesc::TEXTURE_3D:
	{
		D3D11_TEXTURE3D_DESC desc = _ConvertTextureDesc3D(&pTexture->desc);
		hr = device->CreateTexture3D(&desc, data.data(), (ID3D11Texture3D**)&pTexture->resource);
	}
	break;
	default:
		assert(0);
		break;
	}

	assert(SUCCEEDED(hr));
	if (FAILED(hr))
		return SUCCEEDED(hr);

	if (pTexture->desc.MipLevels == 0)
	{
		pTexture->desc.MipLevels = (uint32_t)log2(std::max(pTexture->desc.Width, pTexture->desc.Height));
	}

	if (pTexture->desc.BindFlags & BIND_RENDER_TARGET)
	{
		CreateSubresource(pTexture, RTV, 0, -1, 0, -1);
	}
	if (pTexture->desc.BindFlags & BIND_DEPTH_STENCIL)
	{
		CreateSubresource(pTexture, DSV, 0, -1, 0, -1);
	}
	if (pTexture->desc.BindFlags & BIND_SHADER_RESOURCE)
	{
		CreateSubresource(pTexture, SRV, 0, -1, 0, -1);
	}
	if (pTexture->desc.BindFlags & BIND_UNORDERED_ACCESS)
	{
		CreateSubresource(pTexture, UAV, 0, -1, 0, -1);
	}

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateInputLayout(const VertexLayoutDesc *pInputElementDescs, uint32_t NumElements, const ShaderByteCode* shaderCode, VertexLayout *pInputLayout)
{
	DestroyInputLayout(pInputLayout);
	pInputLayout->Register(this);

	pInputLayout->desc.reserve((size_t)NumElements);

	D3D11_INPUT_ELEMENT_DESC* desc = new D3D11_INPUT_ELEMENT_DESC[NumElements];
	for (uint32_t i = 0; i < NumElements; ++i)
	{
		desc[i].SemanticName = pInputElementDescs[i].SemanticName;
		desc[i].SemanticIndex = pInputElementDescs[i].SemanticIndex;
		desc[i].Format = _ConvertFormat(pInputElementDescs[i].Format);
		desc[i].InputSlot = pInputElementDescs[i].InputSlot;
		desc[i].AlignedByteOffset = pInputElementDescs[i].AlignedByteOffset;
		if (desc[i].AlignedByteOffset == VertexLayoutDesc::APPEND_ALIGNED_ELEMENT)
			desc[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		desc[i].InputSlotClass = _ConvertInputClassification(pInputElementDescs[i].InputSlotClass);
		desc[i].InstanceDataStepRate = pInputElementDescs[i].InstanceDataStepRate;

		pInputLayout->desc.push_back(pInputElementDescs[i]);
	}

	HRESULT hr = device->CreateInputLayout(desc, NumElements, shaderCode->data, shaderCode->size, (ID3D11InputLayout**)&pInputLayout->resource);

	SAFE_DELETE_ARRAY(desc);

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreateVertexShader(const void *pShaderBytecode, size_t BytecodeLength, VertexShader *pVertexShader)
{
	DestroyVertexShader(pVertexShader);
	pVertexShader->Register(this);

	pVertexShader->code.data = new BYTE[BytecodeLength];
	memcpy(pVertexShader->code.data, pShaderBytecode, BytecodeLength);
	pVertexShader->code.size = BytecodeLength;
	return device->CreateVertexShader(pShaderBytecode, BytecodeLength, nullptr, (ID3D11VertexShader**)&pVertexShader->resource);
}
bool GraphicsDevice_DX11::CreatePixelShader(const void *pShaderBytecode, size_t BytecodeLength, PixelShader *pPixelShader)
{
	DestroyPixelShader(pPixelShader);
	pPixelShader->Register(this);

	pPixelShader->code.data = new BYTE[BytecodeLength];
	memcpy(pPixelShader->code.data, pShaderBytecode, BytecodeLength);
	pPixelShader->code.size = BytecodeLength;
	return device->CreatePixelShader(pShaderBytecode, BytecodeLength, nullptr, (ID3D11PixelShader**)&pPixelShader->resource);
}
bool GraphicsDevice_DX11::CreateGeometryShader(const void *pShaderBytecode, size_t BytecodeLength, GeometryShader *pGeometryShader)
{
	DestroyGeometryShader(pGeometryShader);
	pGeometryShader->Register(this);

	pGeometryShader->code.data = new BYTE[BytecodeLength];
	memcpy(pGeometryShader->code.data, pShaderBytecode, BytecodeLength);
	pGeometryShader->code.size = BytecodeLength;
	return device->CreateGeometryShader(pShaderBytecode, BytecodeLength, nullptr, (ID3D11GeometryShader**)&pGeometryShader->resource);
}
bool GraphicsDevice_DX11::CreateHullShader(const void *pShaderBytecode, size_t BytecodeLength, HullShader *pHullShader)
{
	DestroyHullShader(pHullShader);
	pHullShader->Register(this);

	pHullShader->code.data = new BYTE[BytecodeLength];
	memcpy(pHullShader->code.data, pShaderBytecode, BytecodeLength);
	pHullShader->code.size = BytecodeLength;
	return device->CreateHullShader(pShaderBytecode, BytecodeLength, nullptr, (ID3D11HullShader**)&pHullShader->resource);
}
bool GraphicsDevice_DX11::CreateDomainShader(const void *pShaderBytecode, size_t BytecodeLength, DomainShader *pDomainShader)
{
	DestroyDomainShader(pDomainShader);
	pDomainShader->Register(this);

	pDomainShader->code.data = new BYTE[BytecodeLength];
	memcpy(pDomainShader->code.data, pShaderBytecode, BytecodeLength);
	pDomainShader->code.size = BytecodeLength;
	return device->CreateDomainShader(pShaderBytecode, BytecodeLength, nullptr, (ID3D11DomainShader**)&pDomainShader->resource);
}
bool GraphicsDevice_DX11::CreateComputeShader(const void *pShaderBytecode, size_t BytecodeLength, ComputeShader *pComputeShader)
{
	DestroyComputeShader(pComputeShader);
	pComputeShader->Register(this);

	pComputeShader->code.data = new BYTE[BytecodeLength];
	memcpy(pComputeShader->code.data, pShaderBytecode, BytecodeLength);
	pComputeShader->code.size = BytecodeLength;
	return device->CreateComputeShader(pShaderBytecode, BytecodeLength, nullptr, (ID3D11ComputeShader**)&pComputeShader->resource);
}
bool GraphicsDevice_DX11::CreateBlendState(const BlendStateDesc *pBlendStateDesc, BlendState *pBlendState)
{
	DestroyBlendState(pBlendState);
	pBlendState->Register(this);

	D3D11_BLEND_DESC desc;
	desc.AlphaToCoverageEnable = pBlendStateDesc->AlphaToCoverageEnable;
	desc.IndependentBlendEnable = pBlendStateDesc->IndependentBlendEnable;
	for (int i = 0; i < 8; ++i)
	{
		desc.RenderTarget[i].BlendEnable = pBlendStateDesc->RenderTarget[i].BlendEnable;
		desc.RenderTarget[i].SrcBlend = _ConvertBlend(pBlendStateDesc->RenderTarget[i].SrcBlend);
		desc.RenderTarget[i].DestBlend = _ConvertBlend(pBlendStateDesc->RenderTarget[i].DestBlend);
		desc.RenderTarget[i].BlendOp = _ConvertBlendOp(pBlendStateDesc->RenderTarget[i].BlendOp);
		desc.RenderTarget[i].SrcBlendAlpha = _ConvertBlend(pBlendStateDesc->RenderTarget[i].SrcBlendAlpha);
		desc.RenderTarget[i].DestBlendAlpha = _ConvertBlend(pBlendStateDesc->RenderTarget[i].DestBlendAlpha);
		desc.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(pBlendStateDesc->RenderTarget[i].BlendOpAlpha);
		desc.RenderTarget[i].RenderTargetWriteMask = _ParseColorWriteMask(pBlendStateDesc->RenderTarget[i].RenderTargetWriteMask);
	}

	pBlendState->desc = *pBlendStateDesc;
	return device->CreateBlendState(&desc, (ID3D11BlendState**)&pBlendState->resource);
}
bool GraphicsDevice_DX11::CreateDepthStencilState(const DepthStencilStateDesc *pDepthStencilStateDesc, DepthStencilState *pDepthStencilState)
{
	DestroyDepthStencilState(pDepthStencilState);
	pDepthStencilState->Register(this);

	D3D11_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = pDepthStencilStateDesc->DepthEnable;
	desc.DepthWriteMask = _ConvertDepthWriteMask(pDepthStencilStateDesc->DepthWriteMask);
	desc.DepthFunc = _ConvertComparisonFunc(pDepthStencilStateDesc->DepthFunc);
	desc.StencilEnable = pDepthStencilStateDesc->StencilEnable;
	desc.StencilReadMask = pDepthStencilStateDesc->StencilReadMask;
	desc.StencilWriteMask = pDepthStencilStateDesc->StencilWriteMask;
	desc.FrontFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc->FrontFace.StencilDepthFailOp);
	desc.FrontFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc->FrontFace.StencilFailOp);
	desc.FrontFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc->FrontFace.StencilFunc);
	desc.FrontFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc->FrontFace.StencilPassOp);
	desc.BackFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc->BackFace.StencilDepthFailOp);
	desc.BackFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc->BackFace.StencilFailOp);
	desc.BackFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc->BackFace.StencilFunc);
	desc.BackFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc->BackFace.StencilPassOp);

	pDepthStencilState->desc = *pDepthStencilStateDesc;
	return device->CreateDepthStencilState(&desc, (ID3D11DepthStencilState**)&pDepthStencilState->resource);
}
bool GraphicsDevice_DX11::CreateRasterizerState(const RasterizerStateDesc *pRasterizerStateDesc, RasterizerState *pRasterizerState)
{
	DestroyRasterizerState(pRasterizerState);
	pRasterizerState->Register(this);

	pRasterizerState->desc = *pRasterizerStateDesc;

	D3D11_RASTERIZER_DESC desc;
	desc.FillMode = _ConvertFillMode(pRasterizerStateDesc->FillMode);
	desc.CullMode = _ConvertCullMode(pRasterizerStateDesc->CullMode);
	desc.FrontCounterClockwise = pRasterizerStateDesc->FrontCounterClockwise;
	desc.DepthBias = pRasterizerStateDesc->DepthBias;
	desc.DepthBiasClamp = pRasterizerStateDesc->DepthBiasClamp;
	desc.SlopeScaledDepthBias = pRasterizerStateDesc->SlopeScaledDepthBias;
	desc.DepthClipEnable = pRasterizerStateDesc->DepthClipEnable;
	desc.ScissorEnable = true;
	desc.MultisampleEnable = pRasterizerStateDesc->MultisampleEnable;
	desc.AntialiasedLineEnable = pRasterizerStateDesc->AntialiasedLineEnable;


	if (CONSERVATIVE_RASTERIZATION && pRasterizerStateDesc->ConservativeRasterizationEnable == TRUE)
	{
		ID3D11Device3* device3 = nullptr;
		if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)))
		{
			D3D11_RASTERIZER_DESC2 desc2;
			desc2.FillMode = desc.FillMode;
			desc2.CullMode = desc.CullMode;
			desc2.FrontCounterClockwise = desc.FrontCounterClockwise;
			desc2.DepthBias = desc.DepthBias;
			desc2.DepthBiasClamp = desc.DepthBiasClamp;
			desc2.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
			desc2.DepthClipEnable = desc.DepthClipEnable;
			desc2.ScissorEnable = desc.ScissorEnable;
			desc2.MultisampleEnable = desc.MultisampleEnable;
			desc2.AntialiasedLineEnable = desc.AntialiasedLineEnable;
			desc2.ConservativeRaster = D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON;
			desc2.ForcedSampleCount = (RASTERIZER_ORDERED_VIEWS ? pRasterizerStateDesc->ForcedSampleCount : 0);

			pRasterizerState->desc = *pRasterizerStateDesc;

			ID3D11RasterizerState2* rasterizer2 = nullptr;
			HRESULT hr = device3->CreateRasterizerState2(&desc2, &rasterizer2);
			pRasterizerState->resource = (wiCPUHandle)rasterizer2;
			SAFE_RELEASE(device3);
			return SUCCEEDED(hr);
		}
	}
	else if (RASTERIZER_ORDERED_VIEWS && pRasterizerStateDesc->ForcedSampleCount > 0)
	{
		ID3D11Device1* device1 = nullptr;
		if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device1), (void**)&device1)))
		{
			D3D11_RASTERIZER_DESC1 desc1;
			desc1.FillMode = desc.FillMode;
			desc1.CullMode = desc.CullMode;
			desc1.FrontCounterClockwise = desc.FrontCounterClockwise;
			desc1.DepthBias = desc.DepthBias;
			desc1.DepthBiasClamp = desc.DepthBiasClamp;
			desc1.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
			desc1.DepthClipEnable = desc.DepthClipEnable;
			desc1.ScissorEnable = desc.ScissorEnable;
			desc1.MultisampleEnable = desc.MultisampleEnable;
			desc1.AntialiasedLineEnable = desc.AntialiasedLineEnable;
			desc1.ForcedSampleCount = pRasterizerStateDesc->ForcedSampleCount;

			pRasterizerState->desc = *pRasterizerStateDesc;

			ID3D11RasterizerState1* rasterizer1 = nullptr;
			HRESULT hr = device1->CreateRasterizerState1(&desc1, &rasterizer1);
			pRasterizerState->resource = (wiCPUHandle)rasterizer1;
			SAFE_RELEASE(device1);
			return SUCCEEDED(hr);
		}
	}

	return device->CreateRasterizerState(&desc, (ID3D11RasterizerState**)&pRasterizerState->resource);
}
bool GraphicsDevice_DX11::CreateSamplerState(const SamplerDesc *pSamplerDesc, Sampler *pSamplerState)
{
	DestroySamplerState(pSamplerState);
	pSamplerState->Register(this);

	D3D11_SAMPLER_DESC desc;
	desc.Filter = _ConvertFilter(pSamplerDesc->Filter);
	desc.AddressU = _ConvertTextureAddressMode(pSamplerDesc->AddressU);
	desc.AddressV = _ConvertTextureAddressMode(pSamplerDesc->AddressV);
	desc.AddressW = _ConvertTextureAddressMode(pSamplerDesc->AddressW);
	desc.MipLODBias = pSamplerDesc->MipLODBias;
	desc.MaxAnisotropy = pSamplerDesc->MaxAnisotropy;
	desc.ComparisonFunc = _ConvertComparisonFunc(pSamplerDesc->ComparisonFunc);
	desc.BorderColor[0] = pSamplerDesc->BorderColor[0];
	desc.BorderColor[1] = pSamplerDesc->BorderColor[1];
	desc.BorderColor[2] = pSamplerDesc->BorderColor[2];
	desc.BorderColor[3] = pSamplerDesc->BorderColor[3];
	desc.MinLOD = pSamplerDesc->MinLOD;
	desc.MaxLOD = pSamplerDesc->MaxLOD;

	pSamplerState->desc = *pSamplerDesc;
	return device->CreateSamplerState(&desc, (ID3D11SamplerState**)&pSamplerState->resource);
}
bool GraphicsDevice_DX11::CreateQuery(const GPUQueryDesc *pDesc, GPUQuery *pQuery)
{
	DestroyQuery(pQuery);
	pQuery->Register(this);

	HRESULT hr = E_FAIL;

	pQuery->desc = *pDesc;

	D3D11_QUERY_DESC desc;
	desc.MiscFlags = 0;
	desc.Query = D3D11_QUERY_EVENT;
	if (pDesc->Type == GPU_QUERY_TYPE_EVENT)
	{
		desc.Query = D3D11_QUERY_EVENT;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_OCCLUSION)
	{
		desc.Query = D3D11_QUERY_OCCLUSION;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_OCCLUSION_PREDICATE)
	{
		desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_TIMESTAMP)
	{
		desc.Query = D3D11_QUERY_TIMESTAMP;
	}
	else if (pDesc->Type == GPU_QUERY_TYPE_TIMESTAMP_DISJOINT)
	{
		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	}

	hr = device->CreateQuery(&desc, (ID3D11Query**)&pQuery->resource);
	assert(SUCCEEDED(hr) && "GPUQuery creation failed!");

	return SUCCEEDED(hr);
}
bool GraphicsDevice_DX11::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso)
{
	DestroyPipelineState(pso);
	pso->Register(this);

	pso->desc = *pDesc;

	return S_OK;
}
bool GraphicsDevice_DX11::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
{
	DestroyRenderPass(renderpass);
	renderpass->Register(this);

	renderpass->desc = *pDesc;

	return S_OK;
}

int GraphicsDevice_DX11::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	switch (type)
	{
	case wiGraphics::SRV:
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

		// Try to resolve resource format:
		switch (texture->desc.Format)
		{
		case FORMAT_R16_TYPELESS:
			srv_desc.Format = DXGI_FORMAT_R16_UNORM;
			break;
		case FORMAT_R32_TYPELESS:
			srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
			break;
		case FORMAT_R24G8_TYPELESS:
			srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		case FORMAT_R32G8X24_TYPELESS:
			srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			break;
		default:
			srv_desc.Format = _ConvertFormat(texture->desc.Format);
			break;
		}

		if (texture->desc.type == TextureDesc::TEXTURE_1D)
		{
			if (texture->desc.ArraySize > 1)
			{
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
				srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
				srv_desc.Texture1DArray.ArraySize = sliceCount;
				srv_desc.Texture1DArray.MostDetailedMip = firstMip;
				srv_desc.Texture1DArray.MipLevels = mipCount;
			}
			else
			{
				srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
				srv_desc.Texture1D.MostDetailedMip = firstMip;
				srv_desc.Texture1D.MipLevels = mipCount;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				if (texture->desc.MiscFlags & RESOURCE_MISC_TEXTURECUBE)
				{
					if (texture->desc.ArraySize > 6)
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
						srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
						srv_desc.TextureCubeArray.NumCubes = std::min(texture->desc.ArraySize, sliceCount) / 6;
						srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
						srv_desc.TextureCubeArray.MipLevels = mipCount;
					}
					else
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
						srv_desc.TextureCube.MostDetailedMip = firstMip;
						srv_desc.TextureCube.MipLevels = mipCount;
					}
				}
				else
				{
					if (texture->desc.SampleCount > 1)
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
						srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						srv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
						srv_desc.Texture2DArray.FirstArraySlice = firstSlice;
						srv_desc.Texture2DArray.ArraySize = sliceCount;
						srv_desc.Texture2DArray.MostDetailedMip = firstMip;
						srv_desc.Texture2DArray.MipLevels = mipCount;
					}
				}
			}
			else
			{
				if (texture->desc.SampleCount > 1)
				{
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srv_desc.Texture2D.MostDetailedMip = firstMip;
					srv_desc.Texture2D.MipLevels = mipCount;
				}
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_3D)
		{
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			srv_desc.Texture3D.MostDetailedMip = firstMip;
			srv_desc.Texture3D.MipLevels = mipCount;
		}
		ID3D11ShaderResourceView* srv;
		HRESULT hr = device->CreateShaderResourceView((ID3D11Resource*)texture->resource, &srv_desc, &srv);
		if (SUCCEEDED(hr))
		{
			if (texture->SRV == WI_NULL_HANDLE)
			{
				texture->SRV = (wiCPUHandle)srv;
				return -1;
			}
			texture->subresourceSRVs.push_back((wiCPUHandle)srv);
			return int(texture->subresourceSRVs.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	case wiGraphics::UAV:
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

		// Try to resolve resource format:
		switch (texture->desc.Format)
		{
		case FORMAT_R16_TYPELESS:
			uav_desc.Format = DXGI_FORMAT_R16_UNORM;
			break;
		case FORMAT_R32_TYPELESS:
			uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
			break;
		case FORMAT_R24G8_TYPELESS:
			uav_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		case FORMAT_R32G8X24_TYPELESS:
			uav_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			break;
		default:
			uav_desc.Format = _ConvertFormat(texture->desc.Format);
			break;
		}

		if (texture->desc.type == TextureDesc::TEXTURE_1D)
		{
			if (texture->desc.ArraySize > 1)
			{
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
				uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
				uav_desc.Texture1DArray.ArraySize = sliceCount;
				uav_desc.Texture1DArray.MipSlice = firstMip;
			}
			else
			{
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
				uav_desc.Texture1D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
				uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
				uav_desc.Texture2DArray.ArraySize = sliceCount;
				uav_desc.Texture2DArray.MipSlice = firstMip;
			}
			else
			{
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				uav_desc.Texture2D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_3D)
		{
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			uav_desc.Texture3D.MipSlice = firstMip;
			uav_desc.Texture3D.FirstWSlice = 0;
			uav_desc.Texture3D.WSize = -1;
		}
		ID3D11UnorderedAccessView* uav;
		HRESULT hr = device->CreateUnorderedAccessView((ID3D11Resource*)texture->resource, &uav_desc, &uav);
		if (SUCCEEDED(hr))
		{
			if (texture->UAV == WI_NULL_HANDLE)
			{
				texture->UAV = (wiCPUHandle)uav;
				return -1;
			}
			texture->subresourceUAVs.push_back((wiCPUHandle)uav);
			return int(texture->subresourceUAVs.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	case wiGraphics::RTV:
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};

		// Try to resolve resource format:
		switch (texture->desc.Format)
		{
		case FORMAT_R16_TYPELESS:
			rtv_desc.Format = DXGI_FORMAT_R16_UNORM;
			break;
		case FORMAT_R32_TYPELESS:
			rtv_desc.Format = DXGI_FORMAT_R32_FLOAT;
			break;
		case FORMAT_R24G8_TYPELESS:
			rtv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			break;
		case FORMAT_R32G8X24_TYPELESS:
			rtv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			break;
		default:
			rtv_desc.Format = _ConvertFormat(texture->desc.Format);
			break;
		}

		if (texture->desc.type == TextureDesc::TEXTURE_1D)
		{
			if (texture->desc.ArraySize > 1)
			{
				rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
				rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
				rtv_desc.Texture1DArray.ArraySize = sliceCount;
				rtv_desc.Texture1DArray.MipSlice = firstMip;
			}
			else
			{
				rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
				rtv_desc.Texture1D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				if (texture->desc.SampleCount > 1)
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
					rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
					rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
				}
				else
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
					rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
					rtv_desc.Texture2DArray.ArraySize = sliceCount;
					rtv_desc.Texture2DArray.MipSlice = firstMip;
				}
			}
			else
			{
				if (texture->desc.SampleCount > 1)
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					rtv_desc.Texture2D.MipSlice = firstMip;
				}
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_3D)
		{
			rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
			rtv_desc.Texture3D.MipSlice = firstMip;
			rtv_desc.Texture3D.FirstWSlice = 0;
			rtv_desc.Texture3D.WSize = -1;
		}
		ID3D11RenderTargetView* rtv;
		HRESULT hr = device->CreateRenderTargetView((ID3D11Resource*)texture->resource, &rtv_desc, &rtv);
		if (SUCCEEDED(hr))
		{
			if (texture->RTV == WI_NULL_HANDLE)
			{
				texture->RTV = (wiCPUHandle)rtv;
				return -1;
			}
			texture->subresourceRTVs.push_back((wiCPUHandle)rtv);
			return int(texture->subresourceRTVs.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	case wiGraphics::DSV:
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};

		// Try to resolve resource format:
		switch (texture->desc.Format)
		{
		case FORMAT_R16_TYPELESS:
			dsv_desc.Format = DXGI_FORMAT_D16_UNORM;
			break;
		case FORMAT_R32_TYPELESS:
			dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
			break;
		case FORMAT_R24G8_TYPELESS:
			dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			break;
		case FORMAT_R32G8X24_TYPELESS:
			dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			break;
		default:
			dsv_desc.Format = _ConvertFormat(texture->desc.Format);
			break;
		}

		if (texture->desc.type == TextureDesc::TEXTURE_1D)
		{
			if (texture->desc.ArraySize > 1)
			{
				dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
				dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
				dsv_desc.Texture1DArray.ArraySize = sliceCount;
				dsv_desc.Texture1DArray.MipSlice = firstMip;
			}
			else
			{
				dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
				dsv_desc.Texture1D.MipSlice = firstMip;
			}
		}
		else if (texture->desc.type == TextureDesc::TEXTURE_2D)
		{
			if (texture->desc.ArraySize > 1)
			{
				if (texture->desc.SampleCount > 1)
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
					dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
					dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
				}
				else
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
					dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
					dsv_desc.Texture2DArray.ArraySize = sliceCount;
					dsv_desc.Texture2DArray.MipSlice = firstMip;
				}
			}
			else
			{
				if (texture->desc.SampleCount > 1)
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
					dsv_desc.Texture2D.MipSlice = firstMip;
				}
			}
		}
		ID3D11DepthStencilView* dsv;
		HRESULT hr = device->CreateDepthStencilView((ID3D11Resource*)texture->resource, &dsv_desc, &dsv);
		if (SUCCEEDED(hr))
		{
			if (texture->DSV == WI_NULL_HANDLE)
			{
				texture->DSV = (wiCPUHandle)dsv;
				return -1;
			}
			texture->subresourceDSVs.push_back((wiCPUHandle)dsv);
			return int(texture->subresourceDSVs.size() - 1);
		}
		else
		{
			assert(0);
		}
	}
	break;
	default:
		break;
	}
	return -1;
}

void GraphicsDevice_DX11::DestroyResource(GPUResource* pResource)
{
	if (pResource->resource != WI_NULL_HANDLE)
	{
		((ID3D11Resource*)pResource->resource)->Release();
		pResource->resource = WI_NULL_HANDLE;
	}

	if (pResource->SRV != WI_NULL_HANDLE)
	{
		((ID3D11ShaderResourceView*)pResource->SRV)->Release();
		pResource->SRV = WI_NULL_HANDLE;
	}
	for (auto& x : pResource->subresourceSRVs)
	{
		if (x != WI_NULL_HANDLE)
		{
			((ID3D11ShaderResourceView*)x)->Release();
		}
	}
	pResource->subresourceSRVs.clear();

	if (pResource->UAV != WI_NULL_HANDLE)
	{
		((ID3D11UnorderedAccessView*)pResource->UAV)->Release();
		pResource->UAV = WI_NULL_HANDLE;
	}
	for (auto& x : pResource->subresourceUAVs)
	{
		if (x != WI_NULL_HANDLE)
		{
			((ID3D11UnorderedAccessView*)x)->Release();
		}
	}
	pResource->subresourceUAVs.clear();
}
void GraphicsDevice_DX11::DestroyBuffer(GPUBuffer *pBuffer)
{
}
void GraphicsDevice_DX11::DestroyTexture(Texture *pTexture)
{
	if (pTexture->RTV != WI_NULL_HANDLE)
	{
		((ID3D11RenderTargetView*)pTexture->RTV)->Release();
		pTexture->RTV = WI_NULL_HANDLE;
	}
	for (auto& x : pTexture->subresourceRTVs)
	{
		if (x != WI_NULL_HANDLE)
		{
			((ID3D11RenderTargetView*)x)->Release();
		}
	}
	pTexture->subresourceRTVs.clear();

	if (pTexture->DSV != WI_NULL_HANDLE)
	{
		((ID3D11DepthStencilView*)pTexture->DSV)->Release();
		pTexture->DSV = WI_NULL_HANDLE;
	}
	for (auto& x : pTexture->subresourceDSVs)
	{
		if (x != WI_NULL_HANDLE)
		{
			((ID3D11DepthStencilView*)x)->Release();
		}
	}
	pTexture->subresourceDSVs.clear();
}
void GraphicsDevice_DX11::DestroyInputLayout(VertexLayout *pInputLayout)
{
	if (pInputLayout->resource != WI_NULL_HANDLE)
	{
		((ID3D11InputLayout*)pInputLayout->resource)->Release();
		pInputLayout->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyVertexShader(VertexShader *pVertexShader)
{
	if (pVertexShader->resource != WI_NULL_HANDLE)
	{
		((ID3D11VertexShader*)pVertexShader->resource)->Release();
		pVertexShader->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyPixelShader(PixelShader *pPixelShader)
{
	if (pPixelShader->resource != WI_NULL_HANDLE)
	{
		((ID3D11PixelShader*)pPixelShader->resource)->Release();
		pPixelShader->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyGeometryShader(GeometryShader *pGeometryShader)
{
	if (pGeometryShader->resource != WI_NULL_HANDLE)
	{
		((ID3D11GeometryShader*)pGeometryShader->resource)->Release();
		pGeometryShader->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyHullShader(HullShader *pHullShader)
{
	if (pHullShader->resource != WI_NULL_HANDLE)
	{
		((ID3D11HullShader*)pHullShader->resource)->Release();
		pHullShader->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyDomainShader(DomainShader *pDomainShader)
{
	if (pDomainShader->resource != WI_NULL_HANDLE)
	{
		((ID3D11DomainShader*)pDomainShader->resource)->Release();
		pDomainShader->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyComputeShader(ComputeShader *pComputeShader)
{
	if (pComputeShader->resource != WI_NULL_HANDLE)
	{
		((ID3D11ComputeShader*)pComputeShader->resource)->Release();
		pComputeShader->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyBlendState(BlendState *pBlendState)
{
	if (pBlendState->resource != WI_NULL_HANDLE)
	{
		((ID3D11BlendState*)pBlendState->resource)->Release();
		pBlendState->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyDepthStencilState(DepthStencilState *pDepthStencilState)
{
	if (pDepthStencilState->resource != WI_NULL_HANDLE)
	{
		((ID3D11DepthStencilState*)pDepthStencilState->resource)->Release();
		pDepthStencilState->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyRasterizerState(RasterizerState *pRasterizerState)
{
	if (pRasterizerState->resource != WI_NULL_HANDLE)
	{
		((ID3D11RasterizerState*)pRasterizerState->resource)->Release();
		pRasterizerState->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroySamplerState(Sampler *pSamplerState)
{
	if (pSamplerState->resource != WI_NULL_HANDLE)
	{
		((ID3D11SamplerState*)pSamplerState->resource)->Release();
		pSamplerState->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyQuery(GPUQuery *pQuery)
{
	if (pQuery->resource != WI_NULL_HANDLE)
	{
		((ID3D11Query*)pQuery->resource)->Release();
		pQuery->resource = WI_NULL_HANDLE;
	}
}
void GraphicsDevice_DX11::DestroyPipelineState(PipelineState* pso)
{
}
void GraphicsDevice_DX11::DestroyRenderPass(RenderPass* renderpass)
{

}

bool GraphicsDevice_DX11::DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest)
{
	assert(resourceToDownload->type == resourceDest->type);

	if (resourceToDownload->IsBuffer())
	{
		const GPUBuffer* bufferToDownload = static_cast<const GPUBuffer*>(resourceToDownload);
		const GPUBuffer* bufferDest = static_cast<const GPUBuffer*>(resourceDest);

		if (bufferToDownload != nullptr && bufferDest != nullptr)
		{
			assert(bufferToDownload->desc.ByteWidth <= bufferDest->desc.ByteWidth);
			assert(bufferDest->desc.Usage & USAGE_STAGING);
			assert(dataDest != nullptr);

			immediateContext->CopyResource((ID3D11Resource*)bufferDest->resource, (ID3D11Resource*)bufferToDownload->resource);

			D3D11_MAPPED_SUBRESOURCE mappedResource = {};
			HRESULT hr = immediateContext->Map((ID3D11Resource*)bufferDest->resource, 0, D3D11_MAP_READ, /*async ? D3D11_MAP_FLAG_DO_NOT_WAIT :*/ 0, &mappedResource);
			bool result = SUCCEEDED(hr);
			if (result)
			{
				memcpy(dataDest, mappedResource.pData, bufferToDownload->desc.ByteWidth);
				immediateContext->Unmap((ID3D11Resource*)bufferDest->resource, 0);
			}

			return result;
		}
	}
	else if (resourceToDownload->IsTexture())
	{
		const Texture* textureToDownload = static_cast<const Texture*>(resourceToDownload);
		const Texture* textureDest = static_cast<const Texture*>(resourceDest);

		if (textureToDownload != nullptr && textureDest != nullptr)
		{
			assert(textureToDownload->desc.Width <= textureDest->desc.Width);
			assert(textureToDownload->desc.Height <= textureDest->desc.Height);
			assert(textureToDownload->desc.Depth <= textureDest->desc.Depth);
			assert(textureDest->desc.Usage & USAGE_STAGING);
			assert(dataDest != nullptr);

			immediateContext->CopyResource((ID3D11Resource*)textureDest->resource, (ID3D11Resource*)textureToDownload->resource);

			D3D11_MAPPED_SUBRESOURCE mappedResource = {};
			HRESULT hr = immediateContext->Map((ID3D11Resource*)textureDest->resource, 0, D3D11_MAP_READ, 0, &mappedResource);
			bool result = SUCCEEDED(hr);
			if (result)
			{
				uint32_t cpycount = std::max(1u, textureToDownload->desc.Width) * std::max(1u, textureToDownload->desc.Height) * std::max(1u, textureToDownload->desc.Depth);
				uint32_t cpystride = GetFormatStride(textureToDownload->desc.Format);
				uint32_t cpysize = cpycount * cpystride;
				memcpy(dataDest, mappedResource.pData, cpysize);
				immediateContext->Unmap((ID3D11Resource*)textureDest->resource, 0);
			}

			return result;
		}
	}

	return false;
}

void GraphicsDevice_DX11::SetName(GPUResource* pResource, const std::string& name)
{
	((ID3D11Resource*)pResource->resource)->SetPrivateData(WKPDID_D3DDebugObjectName, (uint32_t)name.length(), name.c_str());
}

void GraphicsDevice_DX11::PresentBegin(CommandList cmd)
{
	deviceContexts[cmd]->OMSetRenderTargets(1, &renderTargetView, 0);
	float ClearColor[4] = { 0, 0, 0, 1.0f }; // red,green,blue,alpha
	deviceContexts[cmd]->ClearRenderTargetView(renderTargetView, ClearColor);
}
void GraphicsDevice_DX11::PresentEnd(CommandList cmd)
{
	// Execute deferred command lists:
	{
		CommandList cmd;
		while (active_commandlists.pop_front(cmd))
		{
			deviceContexts[cmd]->FinishCommandList(false, &commandLists[cmd]);
			immediateContext->ExecuteCommandList(commandLists[cmd], false);
			commandLists[cmd]->Release();

			free_commandlists.push_back(cmd);
		}
	}

	swapChain->Present(VSYNC, 0);


	immediateContext->ClearState();

	memset(prev_vs, 0, sizeof(prev_vs));
	memset(prev_ps, 0, sizeof(prev_ps));
	memset(prev_hs, 0, sizeof(prev_hs));
	memset(prev_ds, 0, sizeof(prev_ds));
	memset(prev_gs, 0, sizeof(prev_gs));
	memset(prev_cs, 0, sizeof(prev_cs));
	memset(prev_blendfactor, 0, sizeof(prev_blendfactor));
	memset(prev_samplemask, 0, sizeof(prev_samplemask));
	memset(prev_bs, 0, sizeof(prev_bs));
	memset(prev_rs, 0, sizeof(prev_rs));
	memset(prev_stencilRef, 0, sizeof(prev_stencilRef));
	memset(prev_dss, 0, sizeof(prev_dss));
	memset(prev_il, 0, sizeof(prev_il));
	memset(prev_pt, 0, sizeof(prev_pt));

	memset(raster_uavs, 0, sizeof(raster_uavs));
	memset(raster_uavs_slot, 8, sizeof(raster_uavs_slot));
	memset(raster_uavs_count, 0, sizeof(raster_uavs_count));

	FRAMECOUNT++;

	RESOLUTIONCHANGED = false;
}


CommandList GraphicsDevice_DX11::BeginCommandList()
{
	CommandList cmd;
	if (!free_commandlists.pop_front(cmd))
	{
		// need to create one more command list:
		cmd = (CommandList)commandlist_count.fetch_add(1);
		assert(cmd < COMMANDLIST_COUNT);

		HRESULT hr = device->CreateDeferredContext(0, &deviceContexts[cmd]);
		assert(SUCCEEDED(hr));

		hr = deviceContexts[cmd]->QueryInterface(__uuidof(userDefinedAnnotations[cmd]),
			reinterpret_cast<void**>(&userDefinedAnnotations[cmd]));
		assert(SUCCEEDED(hr));

		// Temporary allocations will use the following buffer type:
		GPUBufferDesc frameAllocatorDesc;
		frameAllocatorDesc.ByteWidth = 4 * 1024 * 1024;
		frameAllocatorDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_INDEX_BUFFER | BIND_VERTEX_BUFFER;
		frameAllocatorDesc.Usage = USAGE_DYNAMIC;
		frameAllocatorDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
		frameAllocatorDesc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		hr = CreateBuffer(&frameAllocatorDesc, nullptr, &frame_allocators[cmd].buffer);
		assert(SUCCEEDED(hr));
		SetName(&frame_allocators[cmd].buffer, "frame_allocator[deferred]");
	}


	BindPipelineState(nullptr, cmd);
	BindComputeShader(nullptr, cmd);

	D3D11_VIEWPORT vp = {};
	vp.Width = (float)SCREENWIDTH;
	vp.Height = (float)SCREENHEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	deviceContexts[cmd]->RSSetViewports(1, &vp);

	D3D11_RECT pRects[8];
	for (uint32_t i = 0; i < 8; ++i)
	{
		pRects[i].bottom = INT32_MAX;
		pRects[i].left = INT32_MIN;
		pRects[i].right = INT32_MAX;
		pRects[i].top = INT32_MIN;
	}
	deviceContexts[cmd]->RSSetScissorRects(8, pRects);

	active_commandlists.push_back(cmd);
	return cmd;
}

void GraphicsDevice_DX11::WaitForGPU()
{
}


void GraphicsDevice_DX11::commit_allocations(CommandList cmd)
{
	// DX11 needs to unmap allocations before it can execute safely

	if (frame_allocators[cmd].dirty)
	{
		deviceContexts[cmd]->Unmap((ID3D11Resource*)frame_allocators[cmd].buffer.resource, 0);
		frame_allocators[cmd].dirty = false;
	}
}


void GraphicsDevice_DX11::RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
{
	const RenderPassDesc& desc = renderpass->GetDesc();

	uint32_t rt_count = 0;
	ID3D11RenderTargetView* RTVs[8] = {};
	ID3D11DepthStencilView* DSV = nullptr;
	for (uint32_t i = 0; i < desc.numAttachments; ++i)
	{
		const RenderPassAttachment& attachment = desc.attachments[i];
		const Texture* texture = attachment.texture;
		int subresource = attachment.subresource;

		if (attachment.type == RenderPassAttachment::RENDERTARGET)
		{
			if (subresource < 0 || texture->subresourceRTVs.empty())
			{
				RTVs[rt_count] = (ID3D11RenderTargetView*)texture->RTV;
			}
			else
			{
				assert(texture->subresourceRTVs.size() > size_t(subresource) && "Invalid RTV subresource!");
				RTVs[rt_count] = (ID3D11RenderTargetView*)texture->subresourceRTVs[subresource];
			}

			if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
			{
				deviceContexts[cmd]->ClearRenderTargetView(RTVs[rt_count], texture->desc.clear.color);
			}

			rt_count++;
		}
		else
		{
			if (subresource < 0 || texture->subresourceDSVs.empty())
			{
				DSV = (ID3D11DepthStencilView*)texture->DSV;
			}
			else
			{
				assert(texture->subresourceDSVs.size() > size_t(subresource) && "Invalid DSV subresource!");
				DSV = (ID3D11DepthStencilView*)texture->subresourceDSVs[subresource];
			}

			if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
			{
				uint32_t _flags = D3D11_CLEAR_DEPTH;
				if (IsFormatStencilSupport(texture->desc.Format))
					_flags |= D3D11_CLEAR_STENCIL;
				deviceContexts[cmd]->ClearDepthStencilView(DSV, _flags, texture->desc.clear.depthstencil.depth, texture->desc.clear.depthstencil.stencil);
			}
		}
	}

	if (raster_uavs_count[cmd] > 0)
	{
		// UAVs:
		const uint32_t count = raster_uavs_count[cmd];
		const uint32_t slot = raster_uavs_slot[cmd];

		deviceContexts[cmd]->OMSetRenderTargetsAndUnorderedAccessViews(rt_count, RTVs, DSV, slot, count, &raster_uavs[cmd][slot], nullptr);

		raster_uavs_count[cmd] = 0;
		raster_uavs_slot[cmd] = 8;
	}
	else
	{
		deviceContexts[cmd]->OMSetRenderTargets(rt_count, RTVs, DSV);
	}
}
void GraphicsDevice_DX11::RenderPassEnd(CommandList cmd)
{
	deviceContexts[cmd]->OMSetRenderTargets(0, nullptr, nullptr);
}
void GraphicsDevice_DX11::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) {
	assert(rects != nullptr);
	assert(numRects <= 8);
	D3D11_RECT pRects[8];
	for(uint32_t i = 0; i < numRects; ++i) {
		pRects[i].bottom = (LONG)rects[i].bottom;
		pRects[i].left = (LONG)rects[i].left;
		pRects[i].right = (LONG)rects[i].right;
		pRects[i].top = (LONG)rects[i].top;
	}
	deviceContexts[cmd]->RSSetScissorRects(numRects, pRects);
}
void GraphicsDevice_DX11::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
{
	assert(NumViewports <= 6);
	D3D11_VIEWPORT d3dViewPorts[6];
	for (uint32_t i = 0; i < NumViewports; ++i)
	{
		d3dViewPorts[i].TopLeftX = pViewports[i].TopLeftX;
		d3dViewPorts[i].TopLeftY = pViewports[i].TopLeftY;
		d3dViewPorts[i].Width = pViewports[i].Width;
		d3dViewPorts[i].Height = pViewports[i].Height;
		d3dViewPorts[i].MinDepth = pViewports[i].MinDepth;
		d3dViewPorts[i].MaxDepth = pViewports[i].MaxDepth;
	}
	deviceContexts[cmd]->RSSetViewports(NumViewports, d3dViewPorts);
}
void GraphicsDevice_DX11::BindResource(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
{
	if (resource != nullptr)
	{
		ID3D11ShaderResourceView* SRV;

		if (subresource < 0)
		{
			SRV = (ID3D11ShaderResourceView*)resource->SRV;
		}
		else
		{
			assert(resource->subresourceSRVs.size() > static_cast<size_t>(subresource) && "Invalid subresource!");
			SRV = (ID3D11ShaderResourceView*)resource->subresourceSRVs[subresource];
		}

		switch (stage)
		{
		case wiGraphics::VS:
			deviceContexts[cmd]->VSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::HS:
			deviceContexts[cmd]->HSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::DS:
			deviceContexts[cmd]->DSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::GS:
			deviceContexts[cmd]->GSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::PS:
			deviceContexts[cmd]->PSSetShaderResources(slot, 1, &SRV);
			break;
		case wiGraphics::CS:
			deviceContexts[cmd]->CSSetShaderResources(slot, 1, &SRV);
			break;
		default:
			assert(0);
			break;
		}
	}
}
void GraphicsDevice_DX11::BindResources(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
{
	assert(count <= 16);
	ID3D11ShaderResourceView* srvs[16];
	for (uint32_t i = 0; i < count; ++i)
	{
		srvs[i] = resources[i] != nullptr ? (ID3D11ShaderResourceView*)resources[i]->SRV : nullptr;
	}

	switch (stage)
	{
	case wiGraphics::VS:
		deviceContexts[cmd]->VSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::HS:
		deviceContexts[cmd]->HSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::DS:
		deviceContexts[cmd]->DSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::GS:
		deviceContexts[cmd]->GSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::PS:
		deviceContexts[cmd]->PSSetShaderResources(slot, count, srvs);
		break;
	case wiGraphics::CS:
		deviceContexts[cmd]->CSSetShaderResources(slot, count, srvs);
		break;
	default:
		assert(0);
		break;
	}
}
void GraphicsDevice_DX11::BindUAV(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
{
	if (resource != nullptr)
	{
		ID3D11UnorderedAccessView* UAV = (ID3D11UnorderedAccessView*)resource->UAV;

		if (subresource < 0)
		{
			UAV = (ID3D11UnorderedAccessView*)resource->UAV;
		}
		else
		{
			assert(resource->subresourceUAVs.size() > static_cast<size_t>(subresource) && "Invalid subresource!");
			UAV = (ID3D11UnorderedAccessView*)resource->subresourceUAVs[subresource];
		}

		if (stage == CS)
		{
			deviceContexts[cmd]->CSSetUnorderedAccessViews(slot, 1, &UAV, nullptr);
		}
		else
		{
			raster_uavs[cmd][slot] = (ID3D11UnorderedAccessView*)resource->UAV;
			raster_uavs_slot[cmd] = std::min(raster_uavs_slot[cmd], uint8_t(slot));
			raster_uavs_count[cmd] = std::max(raster_uavs_count[cmd], uint8_t(1));
		}
	}
}
void GraphicsDevice_DX11::BindUAVs(SHADERSTAGE stage, const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
{
	assert(slot + count <= 8);
	ID3D11UnorderedAccessView* uavs[8];
	for (uint32_t i = 0; i < count; ++i)
	{
		uavs[i] = resources[i] != nullptr ? (ID3D11UnorderedAccessView*)resources[i]->UAV : nullptr;

		if(stage != CS)
		{
			raster_uavs[cmd][slot + i] = uavs[i];
		}
	}

	if(stage == CS)
	{
		deviceContexts[cmd]->CSSetUnorderedAccessViews(static_cast<uint32_t>(slot), static_cast<uint32_t>(count), uavs, nullptr);
	}
	else
	{
		raster_uavs_slot[cmd] = std::min(raster_uavs_slot[cmd], uint8_t(slot));
		raster_uavs_count[cmd] = std::max(raster_uavs_count[cmd], uint8_t(count));
	}
}
void GraphicsDevice_DX11::UnbindResources(uint32_t slot, uint32_t num, CommandList cmd)
{
	assert(num <= arraysize(__nullBlob) && "Extend nullBlob to support more resource unbinding!");
	deviceContexts[cmd]->PSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->VSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->GSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->HSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->DSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
	deviceContexts[cmd]->CSSetShaderResources(slot, num, (ID3D11ShaderResourceView**)__nullBlob);
}
void GraphicsDevice_DX11::UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd)
{
	assert(num <= arraysize(__nullBlob) && "Extend nullBlob to support more resource unbinding!");
	deviceContexts[cmd]->CSSetUnorderedAccessViews(slot, num, (ID3D11UnorderedAccessView**)__nullBlob, 0);

	raster_uavs_count[cmd] = 0;
	raster_uavs_slot[cmd] = 8;
}
void GraphicsDevice_DX11::BindSampler(SHADERSTAGE stage, const Sampler* sampler, uint32_t slot, CommandList cmd)
{
	ID3D11SamplerState* SAM = (ID3D11SamplerState*)sampler->resource;

	switch (stage)
	{
	case wiGraphics::VS:
		deviceContexts[cmd]->VSSetSamplers(slot, 1, &SAM);
		break;
	case wiGraphics::HS:
		deviceContexts[cmd]->HSSetSamplers(slot, 1, &SAM);
		break;
	case wiGraphics::DS:
		deviceContexts[cmd]->DSSetSamplers(slot, 1, &SAM);
		break;
	case wiGraphics::GS:
		deviceContexts[cmd]->GSSetSamplers(slot, 1, &SAM);
		break;
	case wiGraphics::PS:
		deviceContexts[cmd]->PSSetSamplers(slot, 1, &SAM);
		break;
	case wiGraphics::CS:
		deviceContexts[cmd]->CSSetSamplers(slot, 1, &SAM);
		break;
	default:
		assert(0);
		break;
	}
}
void GraphicsDevice_DX11::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd)
{
	ID3D11Buffer* res = buffer ? (ID3D11Buffer*)buffer->resource : nullptr;
	switch (stage)
	{
	case wiGraphics::VS:
		deviceContexts[cmd]->VSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::HS:
		deviceContexts[cmd]->HSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::DS:
		deviceContexts[cmd]->DSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::GS:
		deviceContexts[cmd]->GSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::PS:
		deviceContexts[cmd]->PSSetConstantBuffers(slot, 1, &res);
		break;
	case wiGraphics::CS:
		deviceContexts[cmd]->CSSetConstantBuffers(slot, 1, &res);
		break;
	default:
		assert(0);
		break;
	}
}
void GraphicsDevice_DX11::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd)
{
	assert(count <= 8);
	ID3D11Buffer* res[8] = { 0 };
	for (uint32_t i = 0; i < count; ++i)
	{
		res[i] = vertexBuffers[i] != nullptr ? (ID3D11Buffer*)vertexBuffers[i]->resource : nullptr;
	}
	deviceContexts[cmd]->IASetVertexBuffers(slot, count, res, strides, (offsets != nullptr ? offsets : reinterpret_cast<const uint32_t*>(__nullBlob)));
}
void GraphicsDevice_DX11::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, uint32_t offset, CommandList cmd)
{
	ID3D11Buffer* res = indexBuffer != nullptr ? (ID3D11Buffer*)indexBuffer->resource : nullptr;
	deviceContexts[cmd]->IASetIndexBuffer(res, (format == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT), offset);
}
void GraphicsDevice_DX11::BindStencilRef(uint32_t value, CommandList cmd)
{
	stencilRef[cmd] = value;
}
void GraphicsDevice_DX11::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
{
	blendFactor[cmd].x = r;
	blendFactor[cmd].y = g;
	blendFactor[cmd].z = b;
	blendFactor[cmd].w = a;
}
void GraphicsDevice_DX11::BindPipelineState(const PipelineState* pso, CommandList cmd)
{
	const PipelineStateDesc& desc = pso != nullptr ? pso->GetDesc() : PipelineStateDesc();

	ID3D11VertexShader* vs = desc.vs == nullptr ? nullptr : (ID3D11VertexShader*)desc.vs->resource;
	if (vs != prev_vs[cmd])
	{
		deviceContexts[cmd]->VSSetShader(vs, nullptr, 0);
		prev_vs[cmd] = vs;
	}
	ID3D11PixelShader* ps = desc.ps == nullptr ? nullptr : (ID3D11PixelShader*)desc.ps->resource;
	if (ps != prev_ps[cmd])
	{
		deviceContexts[cmd]->PSSetShader(ps, nullptr, 0);
		prev_ps[cmd] = ps;
	}
	ID3D11HullShader* hs = desc.hs == nullptr ? nullptr : (ID3D11HullShader*)desc.hs->resource;
	if (hs != prev_hs[cmd])
	{
		deviceContexts[cmd]->HSSetShader(hs, nullptr, 0);
		prev_hs[cmd] = hs;
	}
	ID3D11DomainShader* ds = desc.ds == nullptr ? nullptr : (ID3D11DomainShader*)desc.ds->resource;
	if (ds != prev_ds[cmd])
	{
		deviceContexts[cmd]->DSSetShader(ds, nullptr, 0);
		prev_ds[cmd] = ds;
	}
	ID3D11GeometryShader* gs = desc.gs == nullptr ? nullptr : (ID3D11GeometryShader*)desc.gs->resource;
	if (gs != prev_gs[cmd])
	{
		deviceContexts[cmd]->GSSetShader(gs, nullptr, 0);
		prev_gs[cmd] = gs;
	}

	ID3D11BlendState* bs = desc.bs == nullptr ? nullptr : (ID3D11BlendState*)desc.bs->resource;
	if (bs != prev_bs[cmd] || desc.sampleMask != prev_samplemask[cmd] ||
		blendFactor[cmd].x != prev_blendfactor[cmd].x ||
		blendFactor[cmd].y != prev_blendfactor[cmd].y ||
		blendFactor[cmd].z != prev_blendfactor[cmd].z ||
		blendFactor[cmd].w != prev_blendfactor[cmd].w
		)
	{
		const float fact[4] = { blendFactor[cmd].x, blendFactor[cmd].y, blendFactor[cmd].z, blendFactor[cmd].w };
		deviceContexts[cmd]->OMSetBlendState(bs, fact, desc.sampleMask);
		prev_bs[cmd] = bs;
		prev_blendfactor[cmd] = blendFactor[cmd];
		prev_samplemask[cmd] = desc.sampleMask;
	}

	ID3D11RasterizerState* rs = desc.rs == nullptr ? nullptr : (ID3D11RasterizerState*)desc.rs->resource;
	if (rs != prev_rs[cmd])
	{
		deviceContexts[cmd]->RSSetState(rs);
		prev_rs[cmd] = rs;
	}

	ID3D11DepthStencilState* dss = desc.dss == nullptr ? nullptr : (ID3D11DepthStencilState*)desc.dss->resource;
	if (dss != prev_dss[cmd] || stencilRef[cmd] != prev_stencilRef[cmd])
	{
		deviceContexts[cmd]->OMSetDepthStencilState(dss, stencilRef[cmd]);
		prev_dss[cmd] = dss;
		prev_stencilRef[cmd] = stencilRef[cmd];
	}

	ID3D11InputLayout* il = desc.il == nullptr ? nullptr : (ID3D11InputLayout*)desc.il->resource;
	if (il != prev_il[cmd])
	{
		deviceContexts[cmd]->IASetInputLayout(il);
		prev_il[cmd] = il;
	}

	if (prev_pt[cmd] != desc.pt)
	{
		D3D11_PRIMITIVE_TOPOLOGY d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		switch (desc.pt)
		{
		case TRIANGLELIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case TRIANGLESTRIP:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case POINTLIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case LINELIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case LINESTRIP:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case PATCHLIST:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
			break;
		default:
			d3dType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
			break;
		};
		deviceContexts[cmd]->IASetPrimitiveTopology(d3dType);

		prev_pt[cmd] = desc.pt;
	}
}
void GraphicsDevice_DX11::BindComputeShader(const ComputeShader* cs, CommandList cmd)
{
	ID3D11ComputeShader* _cs = cs == nullptr ? nullptr : (ID3D11ComputeShader*)cs->resource;
	if (_cs != prev_cs[cmd])
	{
		deviceContexts[cmd]->CSSetShader(_cs, nullptr, 0);
		prev_cs[cmd] = _cs;
	}
}
void GraphicsDevice_DX11::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) 
{
	commit_allocations(cmd);

	deviceContexts[cmd]->Draw(vertexCount, startVertexLocation);
}
void GraphicsDevice_DX11::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}
void GraphicsDevice_DX11::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) 
{
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}
void GraphicsDevice_DX11::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}
void GraphicsDevice_DX11::DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawInstancedIndirect((ID3D11Buffer*)args->resource, args_offset);
}
void GraphicsDevice_DX11::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->DrawIndexedInstancedIndirect((ID3D11Buffer*)args->resource, args_offset);
}
void GraphicsDevice_DX11::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}
void GraphicsDevice_DX11::DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
{
	commit_allocations(cmd);

	deviceContexts[cmd]->DispatchIndirect((ID3D11Buffer*)args->resource, args_offset);
}
void GraphicsDevice_DX11::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
{
	deviceContexts[cmd]->CopyResource((ID3D11Resource*)pDst->resource, (ID3D11Resource*)pSrc->resource);
}
void GraphicsDevice_DX11::CopyTexture2D_Region(const Texture* pDst, uint32_t dstMip, uint32_t dstX, uint32_t dstY, const Texture* pSrc, uint32_t srcMip, CommandList cmd)
{
	deviceContexts[cmd]->CopySubresourceRegion((ID3D11Resource*)pDst->resource, D3D11CalcSubresource(dstMip, 0, pDst->GetDesc().MipLevels), dstX, dstY, 0,
		(ID3D11Resource*)pSrc->resource, D3D11CalcSubresource(srcMip, 0, pSrc->GetDesc().MipLevels), nullptr);
}
void GraphicsDevice_DX11::MSAAResolve(const Texture* pDst, const Texture* pSrc, CommandList cmd)
{
	assert(pDst != nullptr && pSrc != nullptr);
	deviceContexts[cmd]->ResolveSubresource((ID3D11Resource*)pDst->resource, 0, (ID3D11Resource*)pSrc->resource, 0, _ConvertFormat(pDst->desc.Format));
}
void GraphicsDevice_DX11::UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize)
{
	assert(buffer->desc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
	assert((int)buffer->desc.ByteWidth >= dataSize || dataSize < 0 && "Data size is too big!");

	if (dataSize == 0)
	{
		return;
	}

	dataSize = std::min((int)buffer->desc.ByteWidth, dataSize);

	if (buffer->desc.Usage == USAGE_DYNAMIC)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = deviceContexts[cmd]->Map((ID3D11Resource*)buffer->resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		assert(SUCCEEDED(hr) && "GPUBuffer mapping failed!");
		memcpy(mappedResource.pData, data, (dataSize >= 0 ? dataSize : buffer->desc.ByteWidth));
		deviceContexts[cmd]->Unmap((ID3D11Resource*)buffer->resource, 0);
	}
	else if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER || dataSize < 0)
	{
		deviceContexts[cmd]->UpdateSubresource((ID3D11Resource*)buffer->resource, 0, nullptr, data, 0, 0);
	}
	else
	{
		D3D11_BOX box = {};
		box.left = 0;
		box.right = static_cast<uint32_t>(dataSize);
		box.top = 0;
		box.bottom = 1;
		box.front = 0;
		box.back = 1;
		deviceContexts[cmd]->UpdateSubresource((ID3D11Resource*)buffer->resource, 0, &box, data, 0, 0);
	}
}

void GraphicsDevice_DX11::QueryBegin(const GPUQuery* query, CommandList cmd)
{
	deviceContexts[cmd]->Begin((ID3D11Query*)query->resource);
}
void GraphicsDevice_DX11::QueryEnd(const GPUQuery* query, CommandList cmd)
{
	deviceContexts[cmd]->End((ID3D11Query*)query->resource);
}
bool GraphicsDevice_DX11::QueryRead(const GPUQuery* query, GPUQueryResult* result)
{
	const uint32_t _flags = D3D11_ASYNC_GETDATA_DONOTFLUSH;

	ID3D11Query* QUERY = (ID3D11Query*)query->resource;

	HRESULT hr = S_OK;
	switch (query->desc.Type)
	{
	case GPU_QUERY_TYPE_TIMESTAMP:
		hr = immediateContext->GetData(QUERY, &result->result_timestamp, sizeof(uint64_t), _flags);
		break;
	case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
	{
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT _temp;
		hr = immediateContext->GetData(QUERY, &_temp, sizeof(_temp), _flags);
		result->result_timestamp_frequency = _temp.Frequency;
	}
	break;
	case GPU_QUERY_TYPE_EVENT:
	case GPU_QUERY_TYPE_OCCLUSION:
		hr = immediateContext->GetData(QUERY, &result->result_passed_sample_count, sizeof(uint64_t), _flags);
		break;
	case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
	{
		BOOL passed = FALSE;
		hr = immediateContext->GetData(QUERY, &passed, sizeof(BOOL), _flags);
		result->result_passed_sample_count = (uint64_t)passed;
		break;
	}
	}

	return hr != S_FALSE;
}

GraphicsDevice::GPUAllocation GraphicsDevice_DX11::AllocateGPU(size_t dataSize, CommandList cmd)
{
	GPUAllocator& allocator = frame_allocators[cmd];
	assert(allocator.buffer.desc.ByteWidth > dataSize && "Data of the required size cannot fit!");

	GPUAllocation result;

	if (dataSize == 0)
	{
		return result;
	}

	allocator.dirty = true;


	dataSize = std::min(size_t(allocator.buffer.desc.ByteWidth), dataSize);

	size_t position = allocator.byteOffset;
	bool wrap = position == 0 || position + dataSize > allocator.buffer.desc.ByteWidth || allocator.residentFrame != FRAMECOUNT;
	position = wrap ? 0 : position;

	// Issue buffer rename (realloc) on wrap, otherwise just append data:
	D3D11_MAP mapping = wrap ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = deviceContexts[cmd]->Map((ID3D11Resource*)allocator.buffer.resource, 0, mapping, 0, &mappedResource);
	assert(SUCCEEDED(hr) && "GPUBuffer mapping failed!");

	allocator.byteOffset = position + dataSize;
	allocator.residentFrame = FRAMECOUNT;

	result.buffer = &allocator.buffer;
	result.offset = (uint32_t)position;
	result.data = (void*)((size_t)mappedResource.pData + position);
	return result;
}

void GraphicsDevice_DX11::EventBegin(const std::string& name, CommandList cmd)
{
	userDefinedAnnotations[cmd]->BeginEvent(wstring(name.begin(), name.end()).c_str());
}
void GraphicsDevice_DX11::EventEnd(CommandList cmd)
{
	userDefinedAnnotations[cmd]->EndEvent();
}
void GraphicsDevice_DX11::SetMarker(const std::string& name, CommandList cmd)
{
	userDefinedAnnotations[cmd]->SetMarker(wstring(name.begin(),name.end()).c_str());
}

}
