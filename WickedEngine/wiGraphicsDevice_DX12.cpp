#include "wiGraphicsDevice_DX12.h"
#include "wiGraphicsDevice_SharedInternals.h"
#include "wiHelper.h"
#include "ResourceMapping.h"
#include "wiBackLog.h"

#include "Utility/d3dx12.h"
#include "Utility/D3D12MemAlloc.h"

#include <pix.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"Dxgi.lib")
#pragma comment(lib,"dxguid.lib")

#ifdef _DEBUG
#include <d3d12sdklayers.h>
#endif // _DEBUG

#include <sstream>
#include <algorithm>
#include <wincodec.h>

// Uncomment this to enable DX12 renderpass feature:
//#define DX12_REAL_RENDERPASS

using namespace std;

namespace wiGraphics
{
	// Engine -> Native converters

	inline D3D12_CPU_DESCRIPTOR_HANDLE ToNativeHandle(wiCPUHandle handle)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE native;
		native.ptr = (size_t)handle;
		return native;
	}

	inline uint32_t _ParseColorWriteMask(uint32_t value)
	{
		uint32_t _flag = 0;

		if (value == D3D12_COLOR_WRITE_ENABLE_ALL)
		{
			return D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		else
		{
			if (value & COLOR_WRITE_ENABLE_RED)
				_flag |= D3D12_COLOR_WRITE_ENABLE_RED;
			if (value & COLOR_WRITE_ENABLE_GREEN)
				_flag |= D3D12_COLOR_WRITE_ENABLE_GREEN;
			if (value & COLOR_WRITE_ENABLE_BLUE)
				_flag |= D3D12_COLOR_WRITE_ENABLE_BLUE;
			if (value & COLOR_WRITE_ENABLE_ALPHA)
				_flag |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
		}

		return _flag;
	}

	constexpr D3D12_FILTER _ConvertFilter(FILTER value)
	{
		switch (value)
		{
		case FILTER_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		case FILTER_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_ANISOTROPIC:
			return D3D12_FILTER_ANISOTROPIC;
			break;
		case FILTER_COMPARISON_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_COMPARISON_ANISOTROPIC:
			return D3D12_FILTER_COMPARISON_ANISOTROPIC;
			break;
		case FILTER_MINIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_MINIMUM_ANISOTROPIC:
			return D3D12_FILTER_MINIMUM_ANISOTROPIC;
			break;
		case FILTER_MAXIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case FILTER_MAXIMUM_ANISOTROPIC:
			return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
			break;
		default:
			break;
		}
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}
	constexpr D3D12_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TEXTURE_ADDRESS_MODE value)
	{
		switch (value)
		{
		case TEXTURE_ADDRESS_WRAP:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			break;
		case TEXTURE_ADDRESS_MIRROR:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			break;
		case TEXTURE_ADDRESS_CLAMP:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			break;
		case TEXTURE_ADDRESS_BORDER:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			break;
		case TEXTURE_ADDRESS_MIRROR_ONCE:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
			break;
		default:
			break;
		}
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	}
	constexpr D3D12_COMPARISON_FUNC _ConvertComparisonFunc(COMPARISON_FUNC value)
	{
		switch (value)
		{
		case COMPARISON_NEVER:
			return D3D12_COMPARISON_FUNC_NEVER;
			break;
		case COMPARISON_LESS:
			return D3D12_COMPARISON_FUNC_LESS;
			break;
		case COMPARISON_EQUAL:
			return D3D12_COMPARISON_FUNC_EQUAL;
			break;
		case COMPARISON_LESS_EQUAL:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
			break;
		case COMPARISON_GREATER:
			return D3D12_COMPARISON_FUNC_GREATER;
			break;
		case COMPARISON_NOT_EQUAL:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
			break;
		case COMPARISON_GREATER_EQUAL:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			break;
		case COMPARISON_ALWAYS:
			return D3D12_COMPARISON_FUNC_ALWAYS;
			break;
		default:
			break;
		}
		return D3D12_COMPARISON_FUNC_NEVER;
	}
	constexpr D3D12_FILL_MODE _ConvertFillMode(FILL_MODE value)
	{
		switch (value)
		{
		case FILL_WIREFRAME:
			return D3D12_FILL_MODE_WIREFRAME;
			break;
		case FILL_SOLID:
			return D3D12_FILL_MODE_SOLID;
			break;
		default:
			break;
		}
		return D3D12_FILL_MODE_WIREFRAME;
	}
	constexpr D3D12_CULL_MODE _ConvertCullMode(CULL_MODE value)
	{
		switch (value)
		{
		case CULL_NONE:
			return D3D12_CULL_MODE_NONE;
			break;
		case CULL_FRONT:
			return D3D12_CULL_MODE_FRONT;
			break;
		case CULL_BACK:
			return D3D12_CULL_MODE_BACK;
			break;
		default:
			break;
		}
		return D3D12_CULL_MODE_NONE;
	}
	constexpr D3D12_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DEPTH_WRITE_MASK value)
	{
		switch (value)
		{
		case DEPTH_WRITE_MASK_ZERO:
			return D3D12_DEPTH_WRITE_MASK_ZERO;
			break;
		case DEPTH_WRITE_MASK_ALL:
			return D3D12_DEPTH_WRITE_MASK_ALL;
			break;
		default:
			break;
		}
		return D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	constexpr D3D12_STENCIL_OP _ConvertStencilOp(STENCIL_OP value)
	{
		switch (value)
		{
		case STENCIL_OP_KEEP:
			return D3D12_STENCIL_OP_KEEP;
			break;
		case STENCIL_OP_ZERO:
			return D3D12_STENCIL_OP_ZERO;
			break;
		case STENCIL_OP_REPLACE:
			return D3D12_STENCIL_OP_REPLACE;
			break;
		case STENCIL_OP_INCR_SAT:
			return D3D12_STENCIL_OP_INCR_SAT;
			break;
		case STENCIL_OP_DECR_SAT:
			return D3D12_STENCIL_OP_DECR_SAT;
			break;
		case STENCIL_OP_INVERT:
			return D3D12_STENCIL_OP_INVERT;
			break;
		case STENCIL_OP_INCR:
			return D3D12_STENCIL_OP_INCR;
			break;
		case STENCIL_OP_DECR:
			return D3D12_STENCIL_OP_DECR;
			break;
		default:
			break;
		}
		return D3D12_STENCIL_OP_KEEP;
	}
	constexpr D3D12_BLEND _ConvertBlend(BLEND value)
	{
		switch (value)
		{
		case BLEND_ZERO:
			return D3D12_BLEND_ZERO;
			break;
		case BLEND_ONE:
			return D3D12_BLEND_ONE;
			break;
		case BLEND_SRC_COLOR:
			return D3D12_BLEND_SRC_COLOR;
			break;
		case BLEND_INV_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_COLOR;
			break;
		case BLEND_SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
			break;
		case BLEND_INV_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
			break;
		case BLEND_DEST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
			break;
		case BLEND_INV_DEST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
			break;
		case BLEND_DEST_COLOR:
			return D3D12_BLEND_DEST_COLOR;
			break;
		case BLEND_INV_DEST_COLOR:
			return D3D12_BLEND_INV_DEST_COLOR;
			break;
		case BLEND_SRC_ALPHA_SAT:
			return D3D12_BLEND_SRC_ALPHA_SAT;
			break;
		case BLEND_BLEND_FACTOR:
			return D3D12_BLEND_BLEND_FACTOR;
			break;
		case BLEND_INV_BLEND_FACTOR:
			return D3D12_BLEND_INV_BLEND_FACTOR;
			break;
		case BLEND_SRC1_COLOR:
			return D3D12_BLEND_SRC1_COLOR;
			break;
		case BLEND_INV_SRC1_COLOR:
			return D3D12_BLEND_INV_SRC1_COLOR;
			break;
		case BLEND_SRC1_ALPHA:
			return D3D12_BLEND_SRC1_ALPHA;
			break;
		case BLEND_INV_SRC1_ALPHA:
			return D3D12_BLEND_INV_SRC1_ALPHA;
			break;
		default:
			break;
		}
		return D3D12_BLEND_ZERO;
	}
	constexpr D3D12_BLEND_OP _ConvertBlendOp(BLEND_OP value)
	{
		switch (value)
		{
		case BLEND_OP_ADD:
			return D3D12_BLEND_OP_ADD;
			break;
		case BLEND_OP_SUBTRACT:
			return D3D12_BLEND_OP_SUBTRACT;
			break;
		case BLEND_OP_REV_SUBTRACT:
			return D3D12_BLEND_OP_REV_SUBTRACT;
			break;
		case BLEND_OP_MIN:
			return D3D12_BLEND_OP_MIN;
			break;
		case BLEND_OP_MAX:
			return D3D12_BLEND_OP_MAX;
			break;
		default:
			break;
		}
		return D3D12_BLEND_OP_ADD;
	}
	constexpr D3D12_INPUT_CLASSIFICATION _ConvertInputClassification(INPUT_CLASSIFICATION value)
	{
		switch (value)
		{
		case INPUT_PER_VERTEX_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			break;
		case INPUT_PER_INSTANCE_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			break;
		default:
			break;
		}
		return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
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
	inline D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data;
		data.pData = pInitialData.pSysMem;
		data.RowPitch = pInitialData.SysMemPitch;
		data.SlicePitch = pInitialData.SysMemSlicePitch;

		return data;
	}
	constexpr D3D12_RESOURCE_STATES _ConvertImageLayout(IMAGE_LAYOUT value)
	{
		switch (value)
		{
		case wiGraphics::IMAGE_LAYOUT_UNDEFINED:
		case wiGraphics::IMAGE_LAYOUT_GENERAL:
			return D3D12_RESOURCE_STATE_COMMON;
		case wiGraphics::IMAGE_LAYOUT_RENDERTARGET:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case wiGraphics::IMAGE_LAYOUT_DEPTHSTENCIL_READONLY:
			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case wiGraphics::IMAGE_LAYOUT_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case wiGraphics::IMAGE_LAYOUT_UNORDERED_ACCESS:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case wiGraphics::IMAGE_LAYOUT_COPY_SRC:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case wiGraphics::IMAGE_LAYOUT_COPY_DST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}
	constexpr D3D12_RESOURCE_STATES _ConvertBufferState(BUFFER_STATE value)
	{
		switch (value)
		{
		case wiGraphics::BUFFER_STATE_GENERAL:
			return D3D12_RESOURCE_STATE_COMMON;
		case wiGraphics::BUFFER_STATE_VERTEX_BUFFER:
			return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case wiGraphics::BUFFER_STATE_INDEX_BUFFER:
			return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case wiGraphics::BUFFER_STATE_CONSTANT_BUFFER:
			return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case wiGraphics::BUFFER_STATE_INDIRECT_ARGUMENT:
			return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		case wiGraphics::BUFFER_STATE_SHADER_RESOURCE:
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case wiGraphics::BUFFER_STATE_UNORDERED_ACCESS:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case wiGraphics::BUFFER_STATE_COPY_SRC:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case wiGraphics::BUFFER_STATE_COPY_DST:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}

	// Native -> Engine converters

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

	constexpr TextureDesc _ConvertTextureDesc_Inv(const D3D12_RESOURCE_DESC& desc)
	{
		TextureDesc retVal;

		retVal.Format = _ConvertFormat_Inv(desc.Format);
		retVal.Width = (uint32_t)desc.Width;
		retVal.Height = desc.Height;
		retVal.MipLevels = desc.MipLevels;

		return retVal;
	}


	// Local Helpers:

	inline size_t Align(size_t uLocation, size_t uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			assert(0);
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}

	// Allocator heaps:

	GraphicsDevice_DX12::DescriptorAllocator::DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxCount)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NodeMask = 0;
		heapDesc.NumDescriptors = maxCount;
		heapDesc.Type = type;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&heap);
		assert(SUCCEEDED(hr));

		heap_begin = heap->GetCPUDescriptorHandleForHeapStart().ptr;
		itemSize = device->GetDescriptorHandleIncrementSize(type);
		itemCount = 0;
		this->maxCount = maxCount;

		itemsAlive = new bool[maxCount];
		for (uint32_t i = 0; i < maxCount; ++i)
		{
			itemsAlive[i] = false;
		}
		lastAlloc = 0;
	}
	GraphicsDevice_DX12::DescriptorAllocator::~DescriptorAllocator()
	{
		SAFE_RELEASE(heap);
		SAFE_DELETE_ARRAY(itemsAlive);
	}
	size_t GraphicsDevice_DX12::DescriptorAllocator::allocate()
	{
		size_t addr = 0;

		lock.lock();
		if (itemCount < maxCount - 1)
		{
			while (itemsAlive[lastAlloc] == true)
			{
				lastAlloc = (lastAlloc + 1) % maxCount;
			}
			addr = heap_begin + lastAlloc * itemSize;
			itemsAlive[lastAlloc] = true;

			itemCount++;
		}
		else
		{
			assert(0);
		}
		lock.unlock();

		return addr;
	}
	void GraphicsDevice_DX12::DescriptorAllocator::clear()
	{
		lock.lock();
		for (uint32_t i = 0; i < maxCount; ++i)
		{
			itemsAlive[i] = false;
		}
		itemCount = 0;
		lock.unlock();
	}
	void GraphicsDevice_DX12::DescriptorAllocator::free(wiCPUHandle descriptorHandle)
	{
		if (descriptorHandle == WI_NULL_HANDLE)
		{
			return;
		}

		assert(descriptorHandle >= heap_begin);
		assert(descriptorHandle < heap_begin + maxCount * itemSize);
		uint32_t offset = (uint32_t)(descriptorHandle - heap_begin);
		assert(offset % itemSize == 0);
		offset = offset / itemSize;

		lock.lock();
		itemsAlive[offset] = false;
		assert(itemCount > 0);
		itemCount--;
		lock.unlock();
	}



	GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::DescriptorTableFrameAllocator(GraphicsDevice_DX12* device, uint32_t maxRenameCount_resources, uint32_t maxRenameCount_samplers) : device(device)
	{
		HRESULT hr;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NodeMask = 0;

		// Resource descriptor heap:
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NumDescriptors = (GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + GPU_RESOURCE_HEAP_UAV_COUNT) * SHADERSTAGE_COUNT * maxRenameCount_resources;
		hr = device->device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&resource_heap_GPU);
		assert(SUCCEEDED(hr));

		// Sampler descriptor heap:
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NumDescriptors = GPU_SAMPLER_HEAP_COUNT * SHADERSTAGE_COUNT * maxRenameCount_samplers;
		hr = device->device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&sampler_heap_GPU);
		assert(SUCCEEDED(hr));


		// Save heap properties:
		resource_heap_cpu_start = resource_heap_GPU->GetCPUDescriptorHandleForHeapStart();
		resource_heap_gpu_start = resource_heap_GPU->GetGPUDescriptorHandleForHeapStart();

		sampler_heap_cpu_start = sampler_heap_GPU->GetCPUDescriptorHandleForHeapStart();
		sampler_heap_gpu_start = sampler_heap_GPU->GetGPUDescriptorHandleForHeapStart();


		// Reset state to empty:
		reset();
	}
	GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::~DescriptorTableFrameAllocator()
	{
		SAFE_RELEASE(resource_heap_GPU);
		SAFE_RELEASE(sampler_heap_GPU);
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::reset()
	{
		ringOffset_resources = 0;
		ringOffset_samplers = 0;
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			tables[stage].reset();
		}
	}
	void GraphicsDevice_DX12::FrameResources::DescriptorTableFrameAllocator::validate(CommandList cmd)
	{
		for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
		{
			Table& table = tables[stage];

			if (table.dirty_resources)
			{
				table.dirty_resources = false;

				// First, reset table with null descriptors:
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst = resource_heap_cpu_start;
					dst.ptr += ringOffset_resources;

					device->device->CopyDescriptorsSimple(
						GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + GPU_RESOURCE_HEAP_UAV_COUNT,
						dst,
						device->null_resource_heap_cpu_start,
						D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}

				for (uint32_t slot = 0; slot < GPU_RESOURCE_HEAP_CBV_COUNT; ++slot)
				{
					const GPUBuffer* buffer = table.CBV[slot];
					if (buffer == nullptr)
					{
						continue;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE dst = resource_heap_cpu_start;
					dst.ptr += ringOffset_resources + slot * device->resource_descriptor_size;

					if (buffer->desc.Usage == USAGE_DYNAMIC)
					{
						auto it = device->dynamic_constantbuffers[cmd].find(buffer);
						if (it != device->dynamic_constantbuffers[cmd].end())
						{
							DynamicResourceState& state = it->second;
							state.binding[stage] = true;
							D3D12_CONSTANT_BUFFER_VIEW_DESC cbv;
							cbv.BufferLocation = ((ID3D12Resource*)state.allocation.buffer->resource)->GetGPUVirtualAddress();
							cbv.BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)state.allocation.offset;
							cbv.SizeInBytes = (uint32_t)Align((size_t)buffer->desc.ByteWidth, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

							// Instead of copying like usually, here we create a CBV in place into the GPU-visible table:
							device->device->CreateConstantBufferView(&cbv, dst);
							continue;
						}
					}
					else
					{
						D3D12_CPU_DESCRIPTOR_HANDLE src = ToNativeHandle(buffer->CBV);
						device->device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					}
				}
				for (uint32_t slot = 0; slot < GPU_RESOURCE_HEAP_SRV_COUNT; ++slot)
				{
					const GPUResource* resource = table.SRV[slot];
					const int subresource = table.SRV_index[slot];
					if (resource == nullptr)
					{
						continue;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE src = {};
					if (subresource < 0)
					{
						src = ToNativeHandle(resource->SRV);
					}
					else
					{
						src = ToNativeHandle(resource->subresourceSRVs[subresource]);
					}

					if (src.ptr == 0)
					{
						continue;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE dst = resource_heap_cpu_start;
					dst.ptr += ringOffset_resources + (GPU_RESOURCE_HEAP_CBV_COUNT + slot) * device->resource_descriptor_size;

					device->device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
				for (uint32_t slot = 0; slot < GPU_RESOURCE_HEAP_UAV_COUNT; ++slot)
				{
					const GPUResource* resource = table.UAV[slot];
					const int subresource = table.UAV_index[slot];
					if (resource == nullptr)
					{
						continue;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE src = {};
					if (subresource < 0)
					{
						src = ToNativeHandle(resource->UAV);
					}
					else
					{
						src = ToNativeHandle(resource->subresourceUAVs[subresource]);
					}

					if (src.ptr == 0)
					{
						continue;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE dst = resource_heap_cpu_start;
					dst.ptr += ringOffset_resources + (GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + slot) * device->resource_descriptor_size;

					device->device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}

				// bind table to root sig
				D3D12_GPU_DESCRIPTOR_HANDLE binding_table = resource_heap_gpu_start;
				binding_table.ptr += ringOffset_resources;

				if (stage == CS)
				{
					device->GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(0, binding_table);
				}
				else
				{
					device->GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(stage * 2 + 0, binding_table);
				}

				// allocate next chunk
				ringOffset_resources += (GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + GPU_RESOURCE_HEAP_UAV_COUNT) * device->resource_descriptor_size;
			}

			if (table.dirty_samplers)
			{
				table.dirty_samplers = false;

				// First, reset table with null descriptors:
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst = sampler_heap_cpu_start;
					dst.ptr += ringOffset_samplers;

					device->device->CopyDescriptorsSimple(
						GPU_SAMPLER_HEAP_COUNT,
						dst,
						device->null_sampler_heap_cpu_start,
						D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
				}

				for (uint32_t slot = 0; slot < GPU_SAMPLER_HEAP_COUNT; ++slot)
				{
					const Sampler* sampler = table.SAM[slot];
					if (sampler == nullptr || sampler->resource == WI_NULL_HANDLE)
					{
						continue;
					}

					D3D12_CPU_DESCRIPTOR_HANDLE src = ToNativeHandle(sampler->resource);

					D3D12_CPU_DESCRIPTOR_HANDLE dst = sampler_heap_cpu_start;
					dst.ptr += ringOffset_samplers + slot * device->sampler_descriptor_size;

					device->device->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
				}

				// bind table to root sig
				D3D12_GPU_DESCRIPTOR_HANDLE binding_table = sampler_heap_gpu_start;
				binding_table.ptr += ringOffset_samplers;

				if (stage == CS)
				{
					// compute descriptor heap:
					device->GetDirectCommandList(cmd)->SetComputeRootDescriptorTable(1, binding_table);
				}
				else
				{
					// graphics descriptor heap:
					device->GetDirectCommandList(cmd)->SetGraphicsRootDescriptorTable(stage * 2 + 1, binding_table);
				}

				// allocate next chunk
				ringOffset_samplers += GPU_SAMPLER_HEAP_COUNT * device->sampler_descriptor_size;
			}


		}
	}


	GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::ResourceFrameAllocator(GraphicsDevice_DX12* device, size_t size) : device(device)
	{
		HRESULT hr;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		ID3D12Resource* resource;
		hr = device->allocator->CreateResource(
			&allocationDesc,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			&allocation,
			IID_PPV_ARGS(&resource)
		);
		assert(SUCCEEDED(hr));
		buffer.resource = (wiCPUHandle)resource;

		void* pData;
		CD3DX12_RANGE readRange(0, 0);
		((ID3D12Resource*)buffer.resource)->Map(0, &readRange, &pData);
		dataCur = dataBegin = reinterpret_cast<uint8_t*>(pData);
		dataEnd = dataBegin + size;

		// Because the "buffer" is created by hand in this, fill the desc to indicate how it can be used:
		buffer.desc.ByteWidth = (uint32_t)((size_t)dataEnd - (size_t)dataBegin);
		buffer.desc.Usage = USAGE_DYNAMIC;
		buffer.desc.BindFlags = BIND_VERTEX_BUFFER | BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
		buffer.desc.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	}
	GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::~ResourceFrameAllocator()
	{
		if (buffer.resource != WI_NULL_HANDLE)
		{
			SAFE_RELEASE(allocation);
			((ID3D12Resource*)buffer.resource)->Release();
		}
	}
	uint8_t* GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::allocate(size_t dataSize, size_t alignment)
	{
		dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		if (dataCur + dataSize > dataEnd)
		{
			return nullptr; // failed allocation. TODO: create new heap chunk and allocate from that
		}

		uint8_t* retVal = dataCur;

		dataCur += dataSize;

		return retVal;
	}
	void GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::clear()
	{
		dataCur = dataBegin;
	}
	uint64_t GraphicsDevice_DX12::FrameResources::ResourceFrameAllocator::calculateOffset(uint8_t* address)
	{
		assert(address >= dataBegin && address < dataEnd);
		return static_cast<uint64_t>(address - dataBegin);
	}



	GraphicsDevice_DX12::UploadBuffer::UploadBuffer(GraphicsDevice_DX12* device, size_t size) : device(device)
	{
		HRESULT hr;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		hr = device->allocator->CreateResource(
			&allocationDesc,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			&allocation,
			IID_PPV_ARGS(&resource)
		);
		assert(SUCCEEDED(hr));

		void* pData;
		CD3DX12_RANGE readRange(0, 0);
		resource->Map(0, &readRange, &pData);
		dataCur = dataBegin = reinterpret_cast<UINT8*>(pData);
		dataEnd = dataBegin + size;
	}
	GraphicsDevice_DX12::UploadBuffer::~UploadBuffer()
	{
		SAFE_RELEASE(resource);
		SAFE_RELEASE(allocation);
	}
	uint8_t* GraphicsDevice_DX12::UploadBuffer::allocate(size_t dataSize, size_t alignment)
	{
		lock.lock();

		dataCur = reinterpret_cast<uint8_t*>(Align(reinterpret_cast<size_t>(dataCur), alignment));

		assert(dataCur + dataSize <= dataEnd);

		uint8_t* retVal = dataCur;

		dataCur += dataSize;

		lock.unlock();

		return retVal;
	}
	void GraphicsDevice_DX12::UploadBuffer::clear()
	{
		lock.lock();
		dataCur = dataBegin;
		lock.unlock();
	}
	uint64_t GraphicsDevice_DX12::UploadBuffer::calculateOffset(uint8_t* address)
	{
		assert(address >= dataBegin && address < dataEnd);
		return static_cast<uint64_t>(address - dataBegin);
	}

	// Engine functions
	GraphicsDevice_DX12::GraphicsDevice_DX12(wiWindowRegistration::window_type window, bool fullscreen, bool debuglayer)
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

#if !defined(WINSTORE_SUPPORT)
		if (debuglayer)
		{
			// Enable the debug layer.
			HMODULE dx12 = LoadLibraryEx(L"d3d12.dll",
				nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			auto pD3D12GetDebugInterface =
				reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(
					GetProcAddress(dx12, "D3D12GetDebugInterface"));
			if (pD3D12GetDebugInterface)
			{
				ID3D12Debug* debugController;
				if (SUCCEEDED(pD3D12GetDebugInterface(
					IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();
				}
			}
		}
#endif

		hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device);
		if (FAILED(hr))
		{
			stringstream ss("");
			ss << "Failed to create the graphics device! ERROR: " << std::hex << hr;
			wiHelper::messageBox(ss.str(), "Error!");
			assert(0);
			exit(1);
		}

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = device;

		hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator);
		assert(SUCCEEDED(hr));

		// Create command queue
		D3D12_COMMAND_QUEUE_DESC directQueueDesc = {};
		directQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		directQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		directQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		directQueueDesc.NodeMask = 0;
		hr = device->CreateCommandQueue(&directQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&directQueue);
		assert(SUCCEEDED(hr));

		// Create fences for command queue:
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&frameFence);
		assert(SUCCEEDED(hr));
		frameFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);



		// Create swapchain

		IDXGIFactory4* pIDXGIFactory;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&pIDXGIFactory));
		assert(SUCCEEDED(hr));

		IDXGISwapChain1* _swapChain;

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = SCREENWIDTH;
		sd.Height = SCREENHEIGHT;
		sd.Format = _ConvertFormat(GetBackBufferFormat());
		sd.Stereo = false;
		sd.SampleDesc.Count = 1; // Don't use multi-sampling.
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = BACKBUFFER_COUNT;
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
		hr = pIDXGIFactory->CreateSwapChainForHwnd(directQueue, window, &sd, &fullscreenDesc, nullptr, &_swapChain);
#else
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		sd.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

		hr = pIDXGIFactory->CreateSwapChainForCoreWindow(directQueue, reinterpret_cast<IUnknown*>(window), &sd, nullptr, &_swapChain);
#endif

		if (FAILED(hr))
		{
			wiHelper::messageBox("Failed to create a swapchain for the graphics device!", "Error!");
			assert(0);
			exit(1);
		}

		hr = _swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain);
		assert(SUCCEEDED(hr));

		SAFE_RELEASE(pIDXGIFactory);



		// Create common descriptor heaps
		RTAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
		DSAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1024);
		ResourceAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16384);
		SamplerAllocator = new DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 64);

		resource_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		sampler_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		// Create "null descriptor" heaps:
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NodeMask = 0;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = GPU_RESOURCE_HEAP_CBV_COUNT + GPU_RESOURCE_HEAP_SRV_COUNT + GPU_RESOURCE_HEAP_UAV_COUNT;
		hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&null_resource_heap_CPU);
		assert(SUCCEEDED(hr));

		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NumDescriptors = GPU_SAMPLER_HEAP_COUNT;
		hr = device->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&null_sampler_heap_CPU);
		assert(SUCCEEDED(hr));

		null_sampler_heap_cpu_start = null_sampler_heap_CPU->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE null_sampler_heap_dest = null_sampler_heap_cpu_start;

		null_resource_heap_cpu_start = null_resource_heap_CPU->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE null_resource_heap_dest = null_resource_heap_cpu_start;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		for (uint32_t slot = 0; slot < GPU_RESOURCE_HEAP_CBV_COUNT; ++slot)
		{
			device->CreateConstantBufferView(&cbv_desc, null_resource_heap_dest);
			null_resource_heap_dest.ptr += resource_descriptor_size;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = DXGI_FORMAT_R32_UINT;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		for (uint32_t slot = 0; slot < GPU_RESOURCE_HEAP_SRV_COUNT; ++slot)
		{
			device->CreateShaderResourceView(nullptr, &srv_desc, null_resource_heap_dest);
			null_resource_heap_dest.ptr += resource_descriptor_size;
		}

		D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		uav_desc.Format = DXGI_FORMAT_R32_UINT;
		uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		for (uint32_t slot = 0; slot < GPU_RESOURCE_HEAP_UAV_COUNT; ++slot)
		{
			device->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, null_resource_heap_dest);
			null_resource_heap_dest.ptr += resource_descriptor_size;
		}

		D3D12_SAMPLER_DESC sampler_desc = {};
		sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		for (uint32_t slot = 0; slot < GPU_SAMPLER_HEAP_COUNT; ++slot)
		{
			device->CreateSampler(&sampler_desc, null_sampler_heap_dest);
			null_sampler_heap_dest.ptr += sampler_descriptor_size;
		}

		// Create resource upload buffer
		bufferUploader = new UploadBuffer(this, 256 * 1024 * 1024);
		textureUploader = new UploadBuffer(this, 256 * 1024 * 1024);


		// Create frame-resident resources:
		for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
		{
			hr = swapChain->GetBuffer(fr, __uuidof(ID3D12Resource), (void**)&frames[fr].backBuffer);
			assert(SUCCEEDED(hr));
			frames[fr].backBufferRTV.ptr = RTAllocator->allocate();
			device->CreateRenderTargetView(frames[fr].backBuffer, nullptr, frames[fr].backBufferRTV);
		}


		// Create copy queue:
		{
			D3D12_COMMAND_QUEUE_DESC copyQueueDesc = {};
			copyQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			copyQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			copyQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			copyQueueDesc.NodeMask = 0;
			hr = device->CreateCommandQueue(&copyQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&copyQueue);
			hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, __uuidof(ID3D12CommandAllocator), (void**)&copyAllocator);
			hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, copyAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&copyCommandList);

			hr = static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Close();
			hr = copyAllocator->Reset();
			hr = static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Reset(copyAllocator, nullptr);

			hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&copyFence);
			copyFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
			copyFenceValue = 1;
		}

		// Query features:

		TESSELLATION = true;

		D3D12_FEATURE_DATA_D3D12_OPTIONS features_0;
		hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &features_0, sizeof(features_0));
		CONSERVATIVE_RASTERIZATION = features_0.ConservativeRasterizationTier >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1;
		RASTERIZER_ORDERED_VIEWS = features_0.ROVsSupported == TRUE;

		if (features_0.TypedUAVLoadAdditionalFormats)
		{
			// More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
			UAV_LOAD_FORMAT_COMMON = true;

			D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = { DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
			hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
			if (SUCCEEDED(hr) && (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				UAV_LOAD_FORMAT_R11G11B10_FLOAT = true;
			}
		}


		// Generate default root signature:

		D3D12_DESCRIPTOR_RANGE samplerRange = {};
		samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		samplerRange.BaseShaderRegister = 0;
		samplerRange.RegisterSpace = 0;
		samplerRange.NumDescriptors = GPU_SAMPLER_HEAP_COUNT;
		samplerRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		{
			uint32_t descriptorRangeCount = 3;
			D3D12_DESCRIPTOR_RANGE* descriptorRanges = new D3D12_DESCRIPTOR_RANGE[descriptorRangeCount];

			descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRanges[0].BaseShaderRegister = 0;
			descriptorRanges[0].RegisterSpace = 0;
			descriptorRanges[0].NumDescriptors = GPU_RESOURCE_HEAP_CBV_COUNT;
			descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRanges[1].BaseShaderRegister = 0;
			descriptorRanges[1].RegisterSpace = 0;
			descriptorRanges[1].NumDescriptors = GPU_RESOURCE_HEAP_SRV_COUNT;
			descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRanges[2].BaseShaderRegister = 0;
			descriptorRanges[2].RegisterSpace = 0;
			descriptorRanges[2].NumDescriptors = GPU_RESOURCE_HEAP_UAV_COUNT;
			descriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;




			uint32_t paramCount = 2 * (SHADERSTAGE_COUNT - 1); // 2: resource,sampler;   5: vs,hs,ds,gs,ps;
			D3D12_ROOT_PARAMETER* params = new D3D12_ROOT_PARAMETER[paramCount];
			params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			params[0].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[0].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
			params[1].DescriptorTable.NumDescriptorRanges = 1;
			params[1].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
			params[2].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[2].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
			params[3].DescriptorTable.NumDescriptorRanges = 1;
			params[3].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
			params[4].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[4].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
			params[5].DescriptorTable.NumDescriptorRanges = 1;
			params[5].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
			params[6].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[6].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
			params[7].DescriptorTable.NumDescriptorRanges = 1;
			params[7].DescriptorTable.pDescriptorRanges = &samplerRange;

			params[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			params[8].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[8].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			params[9].DescriptorTable.NumDescriptorRanges = 1;
			params[9].DescriptorTable.pDescriptorRanges = &samplerRange;



			D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.NumParameters = paramCount;
			rootSigDesc.pParameters = params;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ID3DBlob* rootSigBlob;
			ID3DBlob* rootSigError;
			hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
			if (FAILED(hr))
			{
				assert(0);
				OutputDebugStringA((char*)rootSigError->GetBufferPointer());
			}
			hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&graphicsRootSig);
			assert(SUCCEEDED(hr));

			SAFE_DELETE_ARRAY(descriptorRanges);
			SAFE_DELETE_ARRAY(params);
		}
		{
			uint32_t descriptorRangeCount = 3;
			D3D12_DESCRIPTOR_RANGE* descriptorRanges = new D3D12_DESCRIPTOR_RANGE[descriptorRangeCount];

			descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descriptorRanges[0].BaseShaderRegister = 0;
			descriptorRanges[0].RegisterSpace = 0;
			descriptorRanges[0].NumDescriptors = GPU_RESOURCE_HEAP_CBV_COUNT;
			descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRanges[1].BaseShaderRegister = 0;
			descriptorRanges[1].RegisterSpace = 0;
			descriptorRanges[1].NumDescriptors = GPU_RESOURCE_HEAP_SRV_COUNT;
			descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			descriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRanges[2].BaseShaderRegister = 0;
			descriptorRanges[2].RegisterSpace = 0;
			descriptorRanges[2].NumDescriptors = GPU_RESOURCE_HEAP_UAV_COUNT;
			descriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;



			uint32_t paramCount = 2;
			D3D12_ROOT_PARAMETER* params = new D3D12_ROOT_PARAMETER[paramCount];
			params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			params[0].DescriptorTable.NumDescriptorRanges = descriptorRangeCount;
			params[0].DescriptorTable.pDescriptorRanges = descriptorRanges;

			params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			params[1].DescriptorTable.NumDescriptorRanges = 1;
			params[1].DescriptorTable.pDescriptorRanges = &samplerRange;



			D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
			rootSigDesc.NumStaticSamplers = 0;
			rootSigDesc.NumParameters = paramCount;
			rootSigDesc.pParameters = params;
			rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

			ID3DBlob* rootSigBlob;
			ID3DBlob* rootSigError;
			hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &rootSigError);
			if (FAILED(hr))
			{
				assert(0);
				OutputDebugStringA((char*)rootSigError->GetBufferPointer());
			}
			hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&computeRootSig);
			assert(SUCCEEDED(hr));

			SAFE_DELETE_ARRAY(descriptorRanges);
			SAFE_DELETE_ARRAY(params);
		}


		// Create common indirect command signatures:

		D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};

		D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
		dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
		drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

		D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
		drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		cmd_desc.ByteStride = sizeof(IndirectDispatchArgs);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = dispatchArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, __uuidof(ID3D12CommandSignature), (void**)&dispatchIndirectCommandSignature);
		assert(SUCCEEDED(hr));

		cmd_desc.ByteStride = sizeof(IndirectDrawArgsInstanced);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, __uuidof(ID3D12CommandSignature), (void**)&drawInstancedIndirectCommandSignature);
		assert(SUCCEEDED(hr));

		cmd_desc.ByteStride = sizeof(IndirectDrawArgsIndexedInstanced);
		cmd_desc.NumArgumentDescs = 1;
		cmd_desc.pArgumentDescs = drawIndexedInstancedArgs;
		hr = device->CreateCommandSignature(&cmd_desc, nullptr, __uuidof(ID3D12CommandSignature), (void**)&drawIndexedInstancedIndirectCommandSignature);
		assert(SUCCEEDED(hr));

		// GPU Queries:
		{
			D3D12_QUERY_HEAP_DESC queryheapdesc = {};
			queryheapdesc.NodeMask = 0;

			for (uint32_t i = 0; i < timestamp_query_count; ++i)
			{
				free_timestampqueries.push_back(i);
			}
			queryheapdesc.Count = timestamp_query_count;
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			hr = device->CreateQueryHeap(&queryheapdesc, __uuidof(ID3D12QueryHeap), (void**)&querypool_timestamp);
			assert(SUCCEEDED(hr));

			for (uint32_t i = 0; i < occlusion_query_count; ++i)
			{
				free_occlusionqueries.push_back(i);
			}
			queryheapdesc.Count = occlusion_query_count;
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			hr = device->CreateQueryHeap(&queryheapdesc, __uuidof(ID3D12QueryHeap), (void**)&querypool_occlusion);
			assert(SUCCEEDED(hr));


			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;

			hr = allocator->CreateResource(
				&allocationDesc,
				&CD3DX12_RESOURCE_DESC::Buffer(timestamp_query_count * sizeof(uint64_t)),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				&allocation_querypool_timestamp_readback,
				IID_PPV_ARGS(&querypool_timestamp_readback)
			);
			assert(SUCCEEDED(hr));

			hr = allocator->CreateResource(
				&allocationDesc,
				&CD3DX12_RESOURCE_DESC::Buffer(occlusion_query_count * sizeof(uint64_t)),
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				&allocation_querypool_occlusion_readback,
				IID_PPV_ARGS(&querypool_occlusion_readback)
			);
			assert(SUCCEEDED(hr));
		}

		wiBackLog::post("Created GraphicsDevice_DX12");
	}
	GraphicsDevice_DX12::~GraphicsDevice_DX12()
	{
		WaitForGPU();

		SAFE_RELEASE(swapChain);

		for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
		{
			SAFE_RELEASE(frames[fr].backBuffer);

			for (int i = 0; i < COMMANDLIST_COUNT; i++)
			{
				SAFE_RELEASE(frames[fr].commandLists[i]);
				SAFE_RELEASE(frames[fr].commandAllocators[i]);
				SAFE_DELETE(frames[fr].descriptors[i]);
				SAFE_DELETE(frames[fr].resourceBuffer[i]);
			}
		}

		SAFE_RELEASE(frameFence);
		CloseHandle(frameFenceEvent);

		SAFE_RELEASE(copyQueue);
		SAFE_RELEASE(copyAllocator);
		SAFE_RELEASE(copyCommandList);
		SAFE_RELEASE(copyFence);
		CloseHandle(copyFenceEvent);

		SAFE_RELEASE(graphicsRootSig);
		SAFE_RELEASE(computeRootSig);

		SAFE_RELEASE(dispatchIndirectCommandSignature);
		SAFE_RELEASE(drawInstancedIndirectCommandSignature);
		SAFE_RELEASE(drawIndexedInstancedIndirectCommandSignature);

		SAFE_DELETE(RTAllocator);
		SAFE_DELETE(DSAllocator);
		SAFE_DELETE(ResourceAllocator);
		SAFE_DELETE(SamplerAllocator);

		SAFE_RELEASE(null_resource_heap_CPU);
		SAFE_RELEASE(null_sampler_heap_CPU);

		SAFE_DELETE(bufferUploader);
		SAFE_DELETE(textureUploader);

		SAFE_RELEASE(querypool_timestamp);
		SAFE_RELEASE(querypool_occlusion);
		SAFE_RELEASE(querypool_timestamp_readback);
		SAFE_RELEASE(querypool_occlusion_readback);
		SAFE_RELEASE(allocation_querypool_timestamp_readback);
		SAFE_RELEASE(allocation_querypool_occlusion_readback);

		SAFE_RELEASE(directQueue);
		SAFE_RELEASE(allocator);
		SAFE_RELEASE(device);
	}

	void GraphicsDevice_DX12::SetResolution(int width, int height)
	{
		if ((width != SCREENWIDTH || height != SCREENHEIGHT) && width > 0 && height > 0)
		{
			SCREENWIDTH = width;
			SCREENHEIGHT = height;

			WaitForGPU();

			for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				SAFE_RELEASE(frames[fr].backBuffer);
			}

			HRESULT hr = swapChain->ResizeBuffers(GetBackBufferCount(), width, height, _ConvertFormat(GetBackBufferFormat()), 0);
			assert(SUCCEEDED(hr));

			for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				hr = swapChain->GetBuffer(fr, __uuidof(ID3D12Resource), (void**)&frames[fr].backBuffer);
				assert(SUCCEEDED(hr));
				device->CreateRenderTargetView(frames[fr].backBuffer, nullptr, frames[fr].backBufferRTV);
			}

			RESOLUTIONCHANGED = true;
		}
	}

	Texture GraphicsDevice_DX12::GetBackBuffer()
	{
		Texture result;
		result.resource = (wiCPUHandle)GetFrameResources().backBuffer;
		result.RTV = (wiCPUHandle)GetFrameResources().backBufferRTV.ptr;
		return result;
	}

	bool GraphicsDevice_DX12::CreateBuffer(const GPUBufferDesc* pDesc, const SubresourceData* pInitialData, GPUBuffer* pBuffer)
	{
		DestroyBuffer(pBuffer);
		DestroyResource(pBuffer);
		pBuffer->type = GPUResource::GPU_RESOURCE_TYPE::BUFFER;
		pBuffer->Register(this);

		HRESULT hr = E_FAIL;

		pBuffer->desc = *pDesc;

		uint32_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		}
		size_t alignedSize = Align(pDesc->ByteWidth, alignment);

		D3D12_RESOURCE_DESC desc;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Width = (UINT64)alignedSize;
		desc.Height = 1;
		desc.MipLevels = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.DepthOrArraySize = 1;
		desc.Alignment = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		ID3D12Resource* resource;
		D3D12MA::Allocation* allocation;
		hr = allocator->CreateResource(
			&allocationDesc,
			&desc,
			resourceState,
			nullptr,
			&allocation,
			IID_PPV_ARGS(&resource)
		);
		assert(SUCCEEDED(hr));
		pBuffer->resource = (wiCPUHandle)resource;
		destroylocker.lock();
		mem_allocations[pBuffer->resource] = allocation;
		destroylocker.unlock();


		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			copyQueueLock.lock();
			{
				uint8_t* dest = bufferUploader->allocate(pDesc->ByteWidth, 1);
				memcpy(dest, pInitialData->pSysMem, pDesc->ByteWidth);
				static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->CopyBufferRegion(
					(ID3D12Resource*)pBuffer->resource, 0, bufferUploader->resource, bufferUploader->calculateOffset(dest), pDesc->ByteWidth);
			}
			copyQueueLock.unlock();
		}


		// Create resource views if needed
		if (pDesc->BindFlags & BIND_CONSTANT_BUFFER)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
			cbv_desc.SizeInBytes = (uint32_t)alignedSize;
			cbv_desc.BufferLocation = ((ID3D12Resource*)pBuffer->resource)->GetGPUVirtualAddress();

			pBuffer->CBV = ResourceAllocator->allocate();
			device->CreateConstantBufferView(&cbv_desc, ToNativeHandle(pBuffer->CBV));
		}

		if (pDesc->BindFlags & BIND_SHADER_RESOURCE)
		{

			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / 4;
			}
			else if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				srv_desc.Format = DXGI_FORMAT_UNKNOWN;
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
				srv_desc.Buffer.StructureByteStride = pDesc->StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				srv_desc.Format = _ConvertFormat(pDesc->Format);
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = 0;
				srv_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}

			pBuffer->SRV = ResourceAllocator->allocate();
			device->CreateShaderResourceView((ID3D12Resource*)pBuffer->resource, &srv_desc, ToNativeHandle(pBuffer->SRV));
		}

		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
			{
				// This is a Raw Buffer

				uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
				uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
				uav_desc.Buffer.FirstElement = 0;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / 4;
			}
			else if (pDesc->MiscFlags & RESOURCE_MISC_BUFFER_STRUCTURED)
			{
				// This is a Structured Buffer

				uav_desc.Format = DXGI_FORMAT_UNKNOWN;
				uav_desc.Buffer.FirstElement = 0;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
				uav_desc.Buffer.StructureByteStride = pDesc->StructureByteStride;
			}
			else
			{
				// This is a Typed Buffer

				uav_desc.Format = _ConvertFormat(pDesc->Format);
				uav_desc.Buffer.FirstElement = 0;
				uav_desc.Buffer.NumElements = pDesc->ByteWidth / pDesc->StructureByteStride;
			}

			pBuffer->UAV = ResourceAllocator->allocate();
			device->CreateUnorderedAccessView((ID3D12Resource*)pBuffer->resource, nullptr, &uav_desc, ToNativeHandle(pBuffer->UAV));
		}

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture)
	{
		DestroyTexture(pTexture);
		DestroyResource(pTexture);
		pTexture->type = GPUResource::GPU_RESOURCE_TYPE::TEXTURE;
		pTexture->Register(this);

		pTexture->desc = *pDesc;


		HRESULT hr = E_FAIL;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.CreationNodeMask = 0;
		heapDesc.VisibleNodeMask = 0;

		D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

		D3D12_RESOURCE_DESC desc;
		desc.Format = _ConvertFormat(pDesc->Format);
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.MipLevels = pDesc->MipLevels;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.DepthOrArraySize = (UINT16)pDesc->ArraySize;
		desc.SampleDesc.Count = pDesc->SampleCount;
		desc.SampleDesc.Quality = 0;
		desc.Alignment = 0;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (pDesc->BindFlags & BIND_DEPTH_STENCIL)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}
		else if (desc.SampleDesc.Count == 1)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		}
		if (pDesc->BindFlags & BIND_RENDER_TARGET)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}
		if (pDesc->BindFlags & BIND_UNORDERED_ACCESS)
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!(pDesc->BindFlags & BIND_SHADER_RESOURCE))
		{
			desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		D3D12_RESOURCE_STATES resourceState = _ConvertImageLayout(pTexture->desc.layout);

		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Color[0] = pTexture->desc.clear.color[0];
		optimizedClearValue.Color[1] = pTexture->desc.clear.color[1];
		optimizedClearValue.Color[2] = pTexture->desc.clear.color[2];
		optimizedClearValue.Color[3] = pTexture->desc.clear.color[3];
		optimizedClearValue.DepthStencil.Depth = pTexture->desc.clear.depthstencil.depth;
		optimizedClearValue.DepthStencil.Stencil = pTexture->desc.clear.depthstencil.stencil;
		optimizedClearValue.Format = desc.Format;
		if (optimizedClearValue.Format == DXGI_FORMAT_R16_TYPELESS)
		{
			optimizedClearValue.Format = DXGI_FORMAT_D16_UNORM;
		}
		else if (optimizedClearValue.Format == DXGI_FORMAT_R32_TYPELESS)
		{
			optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		}
		else if (optimizedClearValue.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
		{
			optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		}
		bool useClearValue = pDesc->BindFlags & BIND_RENDER_TARGET || pDesc->BindFlags & BIND_DEPTH_STENCIL;

		switch (pTexture->desc.type)
		{
		case TextureDesc::TEXTURE_1D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case TextureDesc::TEXTURE_2D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case TextureDesc::TEXTURE_3D:
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			desc.DepthOrArraySize = (UINT16)pDesc->Depth;
			break;
		default:
			assert(0);
			break;
		}

		ID3D12Resource* resource;
		D3D12MA::Allocation* allocation;
		hr = allocator->CreateResource(
			&allocationDesc,
			&desc,
			resourceState,
			useClearValue ? &optimizedClearValue : nullptr,
			&allocation,
			IID_PPV_ARGS(&resource)
		);
		assert(SUCCEEDED(hr));
		pTexture->resource = (wiCPUHandle)resource;
		destroylocker.lock();
		mem_allocations[pTexture->resource] = allocation;
		destroylocker.unlock();

		if (pTexture->desc.MipLevels == 0)
		{
			pTexture->desc.MipLevels = (uint32_t)log2(std::max(pTexture->desc.Width, pTexture->desc.Height));
		}


		// Issue data copy on request:
		if (pInitialData != nullptr)
		{
			uint32_t dataCount = pDesc->ArraySize * std::max(1u, pDesc->MipLevels);
			D3D12_SUBRESOURCE_DATA* data = new D3D12_SUBRESOURCE_DATA[dataCount];
			for (uint32_t slice = 0; slice < dataCount; ++slice)
			{
				data[slice] = _ConvertSubresourceData(pInitialData[slice]);
			}

			uint32_t FirstSubresource = 0;

			UINT64 RequiredSize = 0;
			device->GetCopyableFootprints(&desc, 0, dataCount, 0, nullptr, nullptr, nullptr, &RequiredSize);


			copyQueueLock.lock();
			{
				uint8_t* dest = textureUploader->allocate(static_cast<size_t>(RequiredSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

				UINT64 dataSize = UpdateSubresources(static_cast<ID3D12GraphicsCommandList*>(copyCommandList), (ID3D12Resource*)pTexture->resource,
					textureUploader->resource, textureUploader->calculateOffset(dest), 0, dataCount, data);
			}
			copyQueueLock.unlock();

			SAFE_DELETE_ARRAY(data);
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
	bool GraphicsDevice_DX12::CreateInputLayout(const VertexLayoutDesc* pInputElementDescs, uint32_t NumElements, const ShaderByteCode* shaderCode, VertexLayout* pInputLayout)
	{
		DestroyInputLayout(pInputLayout);
		pInputLayout->Register(this);

		pInputLayout->desc.clear();
		pInputLayout->desc.reserve((size_t)NumElements);
		for (uint32_t i = 0; i < NumElements; ++i)
		{
			pInputLayout->desc.push_back(pInputElementDescs[i]);
		}

		return S_OK;
	}
	bool GraphicsDevice_DX12::CreateVertexShader(const void* pShaderBytecode, size_t BytecodeLength, VertexShader* pVertexShader)
	{
		DestroyVertexShader(pVertexShader);
		pVertexShader->Register(this);

		pVertexShader->code.data = new BYTE[BytecodeLength];
		memcpy(pVertexShader->code.data, pShaderBytecode, BytecodeLength);
		pVertexShader->code.size = BytecodeLength;

		return (pVertexShader->code.data != nullptr && pVertexShader->code.size > 0 ? S_OK : E_FAIL);
	}
	bool GraphicsDevice_DX12::CreatePixelShader(const void* pShaderBytecode, size_t BytecodeLength, PixelShader* pPixelShader)
	{
		DestroyPixelShader(pPixelShader);
		pPixelShader->Register(this);

		pPixelShader->code.data = new BYTE[BytecodeLength];
		memcpy(pPixelShader->code.data, pShaderBytecode, BytecodeLength);
		pPixelShader->code.size = BytecodeLength;

		return (pPixelShader->code.data != nullptr && pPixelShader->code.size > 0 ? S_OK : E_FAIL);
	}
	bool GraphicsDevice_DX12::CreateGeometryShader(const void* pShaderBytecode, size_t BytecodeLength, GeometryShader* pGeometryShader)
	{
		DestroyGeometryShader(pGeometryShader);
		pGeometryShader->Register(this);

		pGeometryShader->code.data = new BYTE[BytecodeLength];
		memcpy(pGeometryShader->code.data, pShaderBytecode, BytecodeLength);
		pGeometryShader->code.size = BytecodeLength;

		return (pGeometryShader->code.data != nullptr && pGeometryShader->code.size > 0 ? S_OK : E_FAIL);
	}
	bool GraphicsDevice_DX12::CreateHullShader(const void* pShaderBytecode, size_t BytecodeLength, HullShader* pHullShader)
	{
		DestroyHullShader(pHullShader);
		pHullShader->Register(this);

		pHullShader->code.data = new BYTE[BytecodeLength];
		memcpy(pHullShader->code.data, pShaderBytecode, BytecodeLength);
		pHullShader->code.size = BytecodeLength;

		return (pHullShader->code.data != nullptr && pHullShader->code.size > 0 ? S_OK : E_FAIL);
	}
	bool GraphicsDevice_DX12::CreateDomainShader(const void* pShaderBytecode, size_t BytecodeLength, DomainShader* pDomainShader)
	{
		DestroyDomainShader(pDomainShader);
		pDomainShader->Register(this);

		pDomainShader->code.data = new BYTE[BytecodeLength];
		memcpy(pDomainShader->code.data, pShaderBytecode, BytecodeLength);
		pDomainShader->code.size = BytecodeLength;

		return (pDomainShader->code.data != nullptr && pDomainShader->code.size > 0 ? S_OK : E_FAIL);
	}
	bool GraphicsDevice_DX12::CreateComputeShader(const void* pShaderBytecode, size_t BytecodeLength, ComputeShader* pComputeShader)
	{
		DestroyComputeShader(pComputeShader);
		pComputeShader->Register(this);

		pComputeShader->code.data = new BYTE[BytecodeLength];
		memcpy(pComputeShader->code.data, pShaderBytecode, BytecodeLength);
		pComputeShader->code.size = BytecodeLength;

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
		desc.CS.pShaderBytecode = pComputeShader->code.data;
		desc.CS.BytecodeLength = pComputeShader->code.size;
		desc.pRootSignature = computeRootSig;

		HRESULT hr = device->CreateComputePipelineState(&desc, __uuidof(ID3D12PipelineState), (void**)&pComputeShader->resource);
		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreateBlendState(const BlendStateDesc* pBlendStateDesc, BlendState* pBlendState)
	{
		DestroyBlendState(pBlendState);
		pBlendState->Register(this);

		pBlendState->desc = *pBlendStateDesc;
		return S_OK;
	}
	bool GraphicsDevice_DX12::CreateDepthStencilState(const DepthStencilStateDesc* pDepthStencilStateDesc, DepthStencilState* pDepthStencilState)
	{
		DestroyDepthStencilState(pDepthStencilState);
		pDepthStencilState->Register(this);

		pDepthStencilState->desc = *pDepthStencilStateDesc;
		return S_OK;
	}
	bool GraphicsDevice_DX12::CreateRasterizerState(const RasterizerStateDesc* pRasterizerStateDesc, RasterizerState* pRasterizerState)
	{
		DestroyRasterizerState(pRasterizerState);
		pRasterizerState->Register(this);

		pRasterizerState->desc = *pRasterizerStateDesc;
		return S_OK;
	}
	bool GraphicsDevice_DX12::CreateSamplerState(const SamplerDesc* pSamplerDesc, Sampler* pSamplerState)
	{
		DestroySamplerState(pSamplerState);
		pSamplerState->Register(this);

		D3D12_SAMPLER_DESC desc;
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

		pSamplerState->resource = SamplerAllocator->allocate();
		device->CreateSampler(&desc, ToNativeHandle(pSamplerState->resource));

		return S_OK;
	}
	bool GraphicsDevice_DX12::CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery)
	{
		DestroyQuery(pQuery);
		pQuery->Register(this);

		HRESULT hr = E_FAIL;

		pQuery->desc = *pDesc;

		uint32_t query_index;

		switch (pDesc->Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			if (free_timestampqueries.pop_front(query_index))
			{
				pQuery->resource = (wiCPUHandle)query_index;
				hr = S_OK;
			}
			else
			{
				assert(0);
			}
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			hr = S_OK;
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			if (free_occlusionqueries.pop_front(query_index))
			{
				pQuery->resource = (wiCPUHandle)query_index;
				hr = S_OK;
			}
			else
			{
				assert(0);
			}
			break;
		}

		assert(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}
	bool GraphicsDevice_DX12::CreatePipelineState(const PipelineStateDesc* pDesc, PipelineState* pso)
	{
		DestroyPipelineState(pso);
		pso->Register(this);

		pso->desc = *pDesc;

		pso->hash = 0;
		wiHelper::hash_combine(pso->hash, pDesc->vs);
		wiHelper::hash_combine(pso->hash, pDesc->ps);
		wiHelper::hash_combine(pso->hash, pDesc->hs);
		wiHelper::hash_combine(pso->hash, pDesc->ds);
		wiHelper::hash_combine(pso->hash, pDesc->gs);
		wiHelper::hash_combine(pso->hash, pDesc->il);
		wiHelper::hash_combine(pso->hash, pDesc->rs);
		wiHelper::hash_combine(pso->hash, pDesc->bs);
		wiHelper::hash_combine(pso->hash, pDesc->dss);
		wiHelper::hash_combine(pso->hash, pDesc->pt);
		wiHelper::hash_combine(pso->hash, pDesc->sampleMask);

		return S_OK;
	}
	bool GraphicsDevice_DX12::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass)
	{
		DestroyRenderPass(renderpass);
		renderpass->Register(this);

		renderpass->desc = *pDesc;

		renderpass->hash = 0;
		wiHelper::hash_combine(renderpass->hash, pDesc->numAttachments);
		for (uint32_t i = 0; i < pDesc->numAttachments; ++i)
		{
			wiHelper::hash_combine(renderpass->hash, pDesc->attachments[i].texture->desc.Format);
			wiHelper::hash_combine(renderpass->hash, pDesc->attachments[i].texture->desc.SampleCount);
		}

		return S_OK;
	}

	int GraphicsDevice_DX12::CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
	{
		switch (type)
		{
		case wiGraphics::SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

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
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					srv_desc.Texture1DArray.ArraySize = sliceCount;
					srv_desc.Texture1DArray.MostDetailedMip = firstMip;
					srv_desc.Texture1DArray.MipLevels = mipCount;
				}
				else
				{
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
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
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
							srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
							srv_desc.TextureCubeArray.NumCubes = std::min(texture->desc.ArraySize, sliceCount) / 6;
							srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
							srv_desc.TextureCubeArray.MipLevels = mipCount;
						}
						else
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
							srv_desc.TextureCube.MostDetailedMip = firstMip;
							srv_desc.TextureCube.MipLevels = mipCount;
						}
					}
					else
					{
						if (texture->desc.SampleCount > 1)
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
							srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
							srv_desc.Texture2DMSArray.ArraySize = sliceCount;
						}
						else
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
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
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srv_desc.Texture2D.MostDetailedMip = firstMip;
						srv_desc.Texture2D.MipLevels = mipCount;
					}
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_3D)
			{
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srv_desc.Texture3D.MostDetailedMip = firstMip;
				srv_desc.Texture3D.MipLevels = mipCount;
			}
			wiCPUHandle srv = ResourceAllocator->allocate();
			device->CreateShaderResourceView((ID3D12Resource*)texture->resource, &srv_desc, ToNativeHandle(srv));

			if (texture->SRV == WI_NULL_HANDLE)
			{
				texture->SRV = (wiCPUHandle)srv;
				return -1;
			}
			texture->subresourceSRVs.push_back((wiCPUHandle)srv);
			return int(texture->subresourceSRVs.size() - 1);
		}
		break;
		case wiGraphics::UAV:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};

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
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
					uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
					uav_desc.Texture1DArray.ArraySize = sliceCount;
					uav_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
					uav_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_2D)
			{
				if (texture->desc.ArraySize > 1)
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
					uav_desc.Texture2DArray.ArraySize = sliceCount;
					uav_desc.Texture2DArray.MipSlice = firstMip;
				}
				else
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					uav_desc.Texture2D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_3D)
			{
				uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uav_desc.Texture3D.MipSlice = firstMip;
				uav_desc.Texture3D.FirstWSlice = 0;
				uav_desc.Texture3D.WSize = -1;
			}
			wiCPUHandle uav = ResourceAllocator->allocate();
			device->CreateUnorderedAccessView((ID3D12Resource*)texture->resource, nullptr, &uav_desc, ToNativeHandle(uav));

			if (texture->UAV == WI_NULL_HANDLE)
			{
				texture->UAV = (wiCPUHandle)uav;
				return -1;
			}
			texture->subresourceUAVs.push_back((wiCPUHandle)uav);
			return int(texture->subresourceUAVs.size() - 1);
		}
		break;
		case wiGraphics::RTV:
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};

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
					rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
					rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					rtv_desc.Texture1DArray.ArraySize = sliceCount;
					rtv_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
					rtv_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_2D)
			{
				if (texture->desc.ArraySize > 1)
				{
					if (texture->desc.SampleCount > 1)
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
						rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
						rtv_desc.Texture2DArray.ArraySize = sliceCount;
						rtv_desc.Texture2DArray.MipSlice = firstMip;
					}
				}
				else
				{
					if (texture->desc.SampleCount > 1)
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
						rtv_desc.Texture2D.MipSlice = firstMip;
					}
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_3D)
			{
				rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				rtv_desc.Texture3D.MipSlice = firstMip;
				rtv_desc.Texture3D.FirstWSlice = 0;
				rtv_desc.Texture3D.WSize = -1;
			}
			wiCPUHandle rtv = RTAllocator->allocate();
			device->CreateRenderTargetView((ID3D12Resource*)texture->resource, &rtv_desc, ToNativeHandle(rtv));

			if (texture->RTV == WI_NULL_HANDLE)
			{
				texture->RTV = (wiCPUHandle)rtv;
				return -1;
			}
			texture->subresourceRTVs.push_back((wiCPUHandle)rtv);
			return int(texture->subresourceRTVs.size() - 1);
		}
		break;
		case wiGraphics::DSV:
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};

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
					dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
					dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					dsv_desc.Texture1DArray.ArraySize = sliceCount;
					dsv_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
					dsv_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::TEXTURE_2D)
			{
				if (texture->desc.ArraySize > 1)
				{
					if (texture->desc.SampleCount > 1)
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
						dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
						dsv_desc.Texture2DArray.ArraySize = sliceCount;
						dsv_desc.Texture2DArray.MipSlice = firstMip;
					}
				}
				else
				{
					if (texture->desc.SampleCount > 1)
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
						dsv_desc.Texture2D.MipSlice = firstMip;
					}
				}
			}
			wiCPUHandle dsv = DSAllocator->allocate();
			device->CreateDepthStencilView((ID3D12Resource*)texture->resource, &dsv_desc, ToNativeHandle(dsv));

			if (texture->DSV == WI_NULL_HANDLE)
			{
				texture->DSV = (wiCPUHandle)dsv;
				return -1;
			}
			texture->subresourceDSVs.push_back((wiCPUHandle)dsv);
			return int(texture->subresourceDSVs.size() - 1);
		}
		break;
		default:
			break;
		}
		return -1;
	}

	void GraphicsDevice_DX12::DestroyResource(GPUResource* pResource)
	{
		if (pResource->resource != WI_NULL_HANDLE)
		{
			DeferredDestroy({ DestroyItem::RESOURCE, FRAMECOUNT, pResource->resource });
			pResource->resource = WI_NULL_HANDLE;
		}

		DeferredDestroy({ DestroyItem::RESOURCEVIEW, FRAMECOUNT, pResource->SRV });
		pResource->SRV = WI_NULL_HANDLE;
		for (auto& x : pResource->subresourceSRVs)
		{
			DeferredDestroy({ DestroyItem::RESOURCEVIEW, FRAMECOUNT, x });
		}
		pResource->subresourceSRVs.clear();

		DeferredDestroy({ DestroyItem::RESOURCEVIEW, FRAMECOUNT, pResource->UAV });
		pResource->UAV = WI_NULL_HANDLE;
		for (auto& x : pResource->subresourceUAVs)
		{
			DeferredDestroy({ DestroyItem::RESOURCEVIEW, FRAMECOUNT, x });
		}
		pResource->subresourceUAVs.clear();
	}
	void GraphicsDevice_DX12::DestroyBuffer(GPUBuffer* pBuffer)
	{
		DeferredDestroy({ DestroyItem::RESOURCEVIEW, FRAMECOUNT, pBuffer->CBV });
		pBuffer->CBV = WI_NULL_HANDLE;
	}
	void GraphicsDevice_DX12::DestroyTexture(Texture* pTexture)
	{
		DeferredDestroy({ DestroyItem::RENDERTARGETVIEW, FRAMECOUNT, pTexture->RTV });
		pTexture->RTV = WI_NULL_HANDLE;
		for (auto& x : pTexture->subresourceRTVs)
		{
			DeferredDestroy({ DestroyItem::RENDERTARGETVIEW, FRAMECOUNT, x });
		}
		pTexture->subresourceRTVs.clear();

		DeferredDestroy({ DestroyItem::DEPTHSTENCILVIEW, FRAMECOUNT, pTexture->DSV });
		pTexture->DSV = WI_NULL_HANDLE;
		for (auto& x : pTexture->subresourceDSVs)
		{
			DeferredDestroy({ DestroyItem::DEPTHSTENCILVIEW, FRAMECOUNT, x });
		}
		pTexture->subresourceDSVs.clear();
	}
	void GraphicsDevice_DX12::DestroyInputLayout(VertexLayout* pInputLayout)
	{

	}
	void GraphicsDevice_DX12::DestroyVertexShader(VertexShader* pVertexShader)
	{

	}
	void GraphicsDevice_DX12::DestroyPixelShader(PixelShader* pPixelShader)
	{

	}
	void GraphicsDevice_DX12::DestroyGeometryShader(GeometryShader* pGeometryShader)
	{

	}
	void GraphicsDevice_DX12::DestroyHullShader(HullShader* pHullShader)
	{

	}
	void GraphicsDevice_DX12::DestroyDomainShader(DomainShader* pDomainShader)
	{

	}
	void GraphicsDevice_DX12::DestroyComputeShader(ComputeShader* pComputeShader)
	{
		if (pComputeShader->resource != WI_NULL_HANDLE)
		{
			DeferredDestroy({ DestroyItem::PIPELINE, FRAMECOUNT, pComputeShader->resource });
			pComputeShader->resource = WI_NULL_HANDLE;
		}
	}
	void GraphicsDevice_DX12::DestroyBlendState(BlendState* pBlendState)
	{

	}
	void GraphicsDevice_DX12::DestroyDepthStencilState(DepthStencilState* pDepthStencilState)
	{

	}
	void GraphicsDevice_DX12::DestroyRasterizerState(RasterizerState* pRasterizerState)
	{

	}
	void GraphicsDevice_DX12::DestroySamplerState(Sampler* pSamplerState)
	{
		DeferredDestroy({ DestroyItem::SAMPLER, FRAMECOUNT, pSamplerState->resource });
	}
	void GraphicsDevice_DX12::DestroyQuery(GPUQuery* pQuery)
	{
		if (pQuery != nullptr)
		{
			switch (pQuery->desc.Type)
			{
			case GPU_QUERY_TYPE_TIMESTAMP:
				DeferredDestroy({ DestroyItem::QUERY_TIMESTAMP, FRAMECOUNT, pQuery->resource });
				break;
			case GPU_QUERY_TYPE_OCCLUSION:
			case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
				DeferredDestroy({ DestroyItem::QUERY_OCCLUSION, FRAMECOUNT, pQuery->resource });
				break;
			}
			pQuery->desc.Type = GPU_QUERY_TYPE_INVALID;
		}
	}
	void GraphicsDevice_DX12::DestroyPipelineState(PipelineState* pso)
	{
		pso->hash = 0;
	}
	void GraphicsDevice_DX12::DestroyRenderPass(RenderPass* renderpass)
	{

	}

	bool GraphicsDevice_DX12::DownloadResource(const GPUResource* resourceToDownload, const GPUResource* resourceDest, void* dataDest)
	{
		return false;
	}

	void GraphicsDevice_DX12::SetName(GPUResource* pResource, const std::string& name)
	{
		((ID3D12Resource*)pResource->resource)->SetName(wstring(name.begin(), name.end()).c_str());
	}


	void GraphicsDevice_DX12::PresentBegin(CommandList cmd)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = GetFrameResources().backBuffer;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);

		const float clearcolor[] = { 0,0,0,1 };

#ifdef DX12_REAL_RENDERPASS

		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
		RTV.cpuDescriptor = GetFrameResources().backBufferRTV;
		RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		RTV.BeginningAccess.Clear.ClearValue.Color[0] = clearcolor[0];
		RTV.BeginningAccess.Clear.ClearValue.Color[1] = clearcolor[1];
		RTV.BeginningAccess.Clear.ClearValue.Color[2] = clearcolor[2];
		RTV.BeginningAccess.Clear.ClearValue.Color[3] = clearcolor[3];
		RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		GetDirectCommandList(cmd)->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);

#else

		GetDirectCommandList(cmd)->OMSetRenderTargets(1, &GetFrameResources().backBufferRTV, FALSE, nullptr);
		GetDirectCommandList(cmd)->ClearRenderTargetView(GetFrameResources().backBufferRTV, clearcolor, 0, nullptr);

#endif // DX12_REAL_RENDERPASS

	}
	void GraphicsDevice_DX12::PresentEnd(CommandList cmd)
	{
#ifdef DX12_REAL_RENDERPASS
		GetDirectCommandList(cmd)->EndRenderPass();
#endif // DX12_REAL_RENDERPASS

		HRESULT result;

		// Indicate that the back buffer will now be used to present.
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = GetFrameResources().backBuffer;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);



		// Sync up copy queue:
		copyQueueLock.lock();
		{
			static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Close();
			copyQueue->ExecuteCommandLists(1, &copyCommandList);

			// Signal and increment the fence value.
			UINT64 fenceToWaitFor = copyFenceValue;
			HRESULT result = copyQueue->Signal(copyFence, fenceToWaitFor);
			copyFenceValue++;

			// Wait until the GPU is done copying.
			if (copyFence->GetCompletedValue() < fenceToWaitFor)
			{
				result = copyFence->SetEventOnCompletion(fenceToWaitFor, copyFenceEvent);
				WaitForSingleObject(copyFenceEvent, INFINITE);
			}

			result = copyAllocator->Reset();
			result = static_cast<ID3D12GraphicsCommandList*>(copyCommandList)->Reset(copyAllocator, nullptr);

			bufferUploader->clear();
			textureUploader->clear();
		}
		copyQueueLock.unlock();

		// Execute deferred command lists:
		{
			ID3D12CommandList* cmdLists[COMMANDLIST_COUNT];
			CommandList cmds[COMMANDLIST_COUNT];
			uint32_t counter = 0;

			CommandList cmd;
			while (active_commandlists.pop_front(cmd))
			{
				HRESULT hr = GetDirectCommandList(cmd)->Close();
				assert(SUCCEEDED(hr));

				cmdLists[counter] = GetDirectCommandList(cmd);
				cmds[counter] = cmd;
				counter++;

				free_commandlists.push_back(cmd);

				for (auto& x : pipelines_worker[cmd])
				{
					if (pipelines_global.count(x.first) == 0)
					{
						pipelines_global[x.first] = x.second;
					}
					else
					{
						DeferredDestroy({ DestroyItem::PIPELINE, FRAMECOUNT, (wiCPUHandle)x.second });
					}
				}
				pipelines_worker[cmd].clear();
			}

			directQueue->ExecuteCommandLists(counter, cmdLists);
		}


		swapChain->Present(VSYNC, 0);



		// This acts as a barrier, following this we will be using the next frame's resources when calling GetFrameResources()!
		FRAMECOUNT++;



		// Record the latest processed framecount in the fence
		result = directQueue->Signal(frameFence, FRAMECOUNT);

		// Determine the last frame that we should not wait on:
		const uint64_t lastFrameToAllowLatency = std::max(uint64_t(BACKBUFFER_COUNT - 1u), FRAMECOUNT) - (BACKBUFFER_COUNT - 1);

		// Wait if too many frames are being incomplete:
		if (frameFence->GetCompletedValue() < lastFrameToAllowLatency)
		{
			result = frameFence->SetEventOnCompletion(lastFrameToAllowLatency, frameFenceEvent);
			WaitForSingleObject(frameFenceEvent, INFINITE);
		}

		memset(prev_pt, 0, sizeof(prev_pt));


		// Deferred destroy of resources that the GPU is already finished with:
		destroylocker.lock();
		while (!destroyer.empty())
		{
			if (destroyer.front().frame + BACKBUFFER_COUNT < FRAMECOUNT)
			{
				DestroyItem item = destroyer.front();
				destroyer.pop_front();

				switch (item.type)
				{
				case DestroyItem::RESOURCE:
					mem_allocations[item.handle]->Release();
					mem_allocations.erase(item.handle);
					((ID3D12Resource*)item.handle)->Release();
					break;
				case DestroyItem::RESOURCEVIEW:
					ResourceAllocator->free(item.handle);
					break;
				case DestroyItem::RENDERTARGETVIEW:
					RTAllocator->free(item.handle);
					break;
				case DestroyItem::DEPTHSTENCILVIEW:
					DSAllocator->free(item.handle);
					break;
				case DestroyItem::SAMPLER:
					SamplerAllocator->free(item.handle);
					break;
				case DestroyItem::PIPELINE:
					((ID3D12PipelineState*)item.handle)->Release();
					break;
				case DestroyItem::QUERY_TIMESTAMP:
					free_timestampqueries.push_back((uint32_t)item.handle);
					break;
				case DestroyItem::QUERY_OCCLUSION:
					free_timestampqueries.push_back((uint32_t)item.handle);
					break;
				default:
					break;
				}
			}
			else
			{
				break;
			}
		}
		destroylocker.unlock();

		RESOLUTIONCHANGED = false;
	}

	CommandList GraphicsDevice_DX12::BeginCommandList()
	{
		CommandList cmd;
		if (!free_commandlists.pop_front(cmd))
		{
			// need to create one more command list:
			cmd = commandlist_count.fetch_add(1);
			assert(cmd < COMMANDLIST_COUNT);

			HRESULT hr;
			for (uint32_t fr = 0; fr < BACKBUFFER_COUNT; ++fr)
			{
				hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&frames[fr].commandAllocators[cmd]);
				hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[fr].commandAllocators[cmd], nullptr, __uuidof(ID3D12GraphicsCommandList4), (void**)&frames[fr].commandLists[cmd]);
				hr = static_cast<ID3D12GraphicsCommandList4*>(frames[fr].commandLists[cmd])->Close();

				frames[fr].descriptors[cmd] = new FrameResources::DescriptorTableFrameAllocator(this, 1024, 16);
				frames[fr].resourceBuffer[cmd] = new FrameResources::ResourceFrameAllocator(this, 1024 * 1024 * 4);
			}
		}


		// Start the command list in a default state:

		HRESULT hr = GetFrameResources().commandAllocators[cmd]->Reset();
		assert(SUCCEEDED(hr));
		hr = GetDirectCommandList(cmd)->Reset(GetFrameResources().commandAllocators[cmd], nullptr);
		assert(SUCCEEDED(hr));

		ID3D12DescriptorHeap* heaps[] = {
			GetFrameResources().descriptors[cmd]->resource_heap_GPU, GetFrameResources().descriptors[cmd]->sampler_heap_GPU
		};
		GetDirectCommandList(cmd)->SetDescriptorHeaps(arraysize(heaps), heaps);

		GetDirectCommandList(cmd)->SetGraphicsRootSignature(graphicsRootSig);
		GetDirectCommandList(cmd)->SetComputeRootSignature(computeRootSig);

		GetFrameResources().descriptors[cmd]->reset();
		GetFrameResources().resourceBuffer[cmd]->clear();

		D3D12_VIEWPORT vp = {};
		vp.Width = (float)SCREENWIDTH;
		vp.Height = (float)SCREENHEIGHT;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		GetDirectCommandList(cmd)->RSSetViewports(1, &vp);

		D3D12_RECT pRects[8];
		for (uint32_t i = 0; i < 8; ++i)
		{
			pRects[i].bottom = INT32_MAX;
			pRects[i].left = INT32_MIN;
			pRects[i].right = INT32_MAX;
			pRects[i].top = INT32_MIN;
		}
		GetDirectCommandList(cmd)->RSSetScissorRects(8, pRects);

		prev_pipeline_hash[cmd] = 0;
		active_renderpass[cmd] = nullptr;

		active_commandlists.push_back(cmd);
		return cmd;
	}

	void GraphicsDevice_DX12::WaitForGPU()
	{
		if (frameFence->GetCompletedValue() < FRAMECOUNT)
		{
			HRESULT result = frameFence->SetEventOnCompletion(FRAMECOUNT, frameFenceEvent);
			WaitForSingleObject(frameFenceEvent, INFINITE);
		}
	}
	void GraphicsDevice_DX12::ClearPipelineStateCache()
	{
		for (auto& x : pipelines_global)
		{
			DeferredDestroy({ DestroyItem::PIPELINE, FRAMECOUNT, (wiCPUHandle)x.second });
		}
		pipelines_global.clear();

		for (int i = 0; i < arraysize(pipelines_worker); ++i)
		{
			for (auto& x : pipelines_worker[i])
			{
				DeferredDestroy({ DestroyItem::PIPELINE, FRAMECOUNT, (wiCPUHandle)x.second });
			}
			pipelines_worker[i].clear();
		}
	}

	void GraphicsDevice_DX12::RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
	{
		active_renderpass[cmd] = renderpass;

		const RenderPassDesc& desc = renderpass->GetDesc();

#ifdef DX12_REAL_RENDERPASS

		uint32_t rt_count = 0;
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTVs[8] = {};
		bool dsv = false;
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};
		for (uint32_t i = 0; i < desc.numAttachments; ++i)
		{
			const RenderPassAttachment& attachment = desc.attachments[i];
			const Texture2D* texture = attachment.texture;
			int subresource = attachment.subresource;

			D3D12_CLEAR_VALUE clear_value;
			clear_value.Format = _ConvertFormat(texture->desc.Format);

			if (attachment.type == RenderPassAttachment::RENDERTARGET)
			{
				if (subresource < 0 || texture->subresourceRTVs.empty())
				{
					RTVs[rt_count].cpuDescriptor = ToNativeHandle(texture->RTV);
				}
				else
				{
					assert(texture->subresourceRTVs.size() > size_t(subresource) && "Invalid RTV subresource!");
					RTVs[rt_count].cpuDescriptor = ToNativeHandle(texture->subresourceRTVs[subresource]);
				}

				switch (attachment.loadop)
				{
				default:
				case RenderPassAttachment::LOADOP_LOAD:
					RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LOADOP_CLEAR:
					RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.Color[0] = texture->desc.clear.color[0];
					clear_value.Color[1] = texture->desc.clear.color[1];
					clear_value.Color[2] = texture->desc.clear.color[2];
					clear_value.Color[3] = texture->desc.clear.color[3];
					RTVs[rt_count].BeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LOADOP_DONTCARE:
					RTVs[rt_count].BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::STOREOP_STORE:
					RTVs[rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::STOREOP_DONTCARE:
					RTVs[rt_count].EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}

				rt_count++;
			}
			else
			{
				dsv = true;
				if (subresource < 0 || texture->subresourceDSVs.empty())
				{
					DSV.cpuDescriptor = ToNativeHandle(texture->DSV);
				}
				else
				{
					assert(texture->subresourceDSVs.size() > size_t(subresource) && "Invalid DSV subresource!");
					DSV.cpuDescriptor = ToNativeHandle(texture->subresourceDSVs[subresource]);
				}

				switch (attachment.loadop)
				{
				default:
				case RenderPassAttachment::LOADOP_LOAD:
					DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::LOADOP_CLEAR:
					DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
					clear_value.DepthStencil.Depth = texture->desc.clear.depthstencil.depth;
					clear_value.DepthStencil.Stencil = texture->desc.clear.depthstencil.stencil;
					DSV.DepthBeginningAccess.Clear.ClearValue = clear_value;
					DSV.StencilBeginningAccess.Clear.ClearValue = clear_value;
					break;
				case RenderPassAttachment::LOADOP_DONTCARE:
					DSV.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
					break;
				}

				switch (attachment.storeop)
				{
				default:
				case RenderPassAttachment::STOREOP_STORE:
					DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
					break;
				case RenderPassAttachment::STOREOP_DONTCARE:
					DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
					break;
				}
			}
		}

		GetDirectCommandList(cmd)->BeginRenderPass(rt_count, RTVs, dsv ? &DSV : nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);

#else

		uint32_t rt_count = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE RTVs[8] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE* DSV = nullptr;
		for (uint32_t i = 0; i < desc.numAttachments; ++i)
		{
			const RenderPassAttachment& attachment = desc.attachments[i];
			const Texture* texture = attachment.texture;
			int subresource = attachment.subresource;

			if (attachment.type == RenderPassAttachment::RENDERTARGET)
			{
				if (subresource < 0 || texture->subresourceRTVs.empty())
				{
					RTVs[rt_count] = ToNativeHandle(texture->RTV);
				}
				else
				{
					assert(texture->subresourceRTVs.size() > size_t(subresource) && "Invalid RTV subresource!");
					RTVs[rt_count] = ToNativeHandle(texture->subresourceRTVs[subresource]);
				}

				if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
				{
					GetDirectCommandList(cmd)->ClearRenderTargetView(RTVs[rt_count], texture->desc.clear.color, 0, nullptr);
				}

				rt_count++;
			}
			else
			{
				if (subresource < 0 || texture->subresourceDSVs.empty())
				{
					DSV = &ToNativeHandle(texture->DSV);
				}
				else
				{
					assert(texture->subresourceDSVs.size() > size_t(subresource) && "Invalid DSV subresource!");
					DSV = &ToNativeHandle(texture->subresourceDSVs[subresource]);
				}

				if (attachment.loadop == RenderPassAttachment::LOADOP_CLEAR)
				{
					uint32_t _flags = D3D12_CLEAR_FLAG_DEPTH;
					if (IsFormatStencilSupport(texture->desc.Format))
						_flags |= D3D12_CLEAR_FLAG_STENCIL;
					GetDirectCommandList(cmd)->ClearDepthStencilView(*DSV, (D3D12_CLEAR_FLAGS)_flags, texture->desc.clear.depthstencil.depth, texture->desc.clear.depthstencil.stencil, 0, nullptr);
				}
			}
		}

		GetDirectCommandList(cmd)->OMSetRenderTargets(rt_count, RTVs, FALSE, DSV);

#endif // DX12_REAL_RENDERPASS
	}
	void GraphicsDevice_DX12::RenderPassEnd(CommandList cmd)
	{
#ifdef DX12_REAL_RENDERPASS
		GetDirectCommandList(cmd)->EndRenderPass();
#else
		GetDirectCommandList(cmd)->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
#endif // DX12_REAL_RENDERPASS



		// Perform render pass transitions:
		D3D12_RESOURCE_BARRIER barrierdescs[9];
		uint32_t numBarriers = 0;
		for (uint32_t i = 0; i < active_renderpass[cmd]->desc.numAttachments; ++i)
		{
			const RenderPassAttachment& attachment = active_renderpass[cmd]->desc.attachments[i];
			if (attachment.initial_layout == attachment.final_layout)
			{
				continue;
			}

			D3D12_RESOURCE_BARRIER& barrierdesc = barrierdescs[numBarriers++];

			barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrierdesc.Transition.pResource = (ID3D12Resource*)attachment.texture->resource;
			barrierdesc.Transition.StateBefore = _ConvertImageLayout(attachment.initial_layout);
			barrierdesc.Transition.StateAfter = _ConvertImageLayout(attachment.final_layout);
			barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		}
		if (numBarriers > 0)
		{
			GetDirectCommandList(cmd)->ResourceBarrier(numBarriers, barrierdescs);
		}

		active_renderpass[cmd] = nullptr;
	}
	void GraphicsDevice_DX12::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) {
		assert(rects != nullptr);
		assert(numRects <= 8);
		D3D12_RECT pRects[8];
		for (uint32_t i = 0; i < numRects; ++i) {
			pRects[i].bottom = (LONG)rects[i].bottom;
			pRects[i].left = (LONG)rects[i].left;
			pRects[i].right = (LONG)rects[i].right;
			pRects[i].top = (LONG)rects[i].top;
		}
		GetDirectCommandList(cmd)->RSSetScissorRects(numRects, pRects);
	}
	void GraphicsDevice_DX12::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(NumViewports <= 6);
		D3D12_VIEWPORT d3dViewPorts[6];
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			d3dViewPorts[i].TopLeftX = pViewports[i].TopLeftX;
			d3dViewPorts[i].TopLeftY = pViewports[i].TopLeftY;
			d3dViewPorts[i].Width = pViewports[i].Width;
			d3dViewPorts[i].Height = pViewports[i].Height;
			d3dViewPorts[i].MinDepth = pViewports[i].MinDepth;
			d3dViewPorts[i].MaxDepth = pViewports[i].MaxDepth;
		}
		GetDirectCommandList(cmd)->RSSetViewports(NumViewports, d3dViewPorts);
	}
	void GraphicsDevice_DX12::BindResource(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < GPU_RESOURCE_HEAP_SRV_COUNT);
		auto& table = GetFrameResources().descriptors[cmd]->tables[stage];
		if (table.SRV[slot] != resource || table.SRV_index[slot] != subresource)
		{
			table.SRV[slot] = resource;
			table.SRV_index[slot] = subresource;
			table.dirty_resources = true;
		}
	}
	void GraphicsDevice_DX12::BindResources(SHADERSTAGE stage, const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(stage, resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_DX12::BindUAV(SHADERSTAGE stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < GPU_RESOURCE_HEAP_UAV_COUNT);
		auto& table = GetFrameResources().descriptors[cmd]->tables[stage];
		if (table.UAV[slot] != resource || table.UAV_index[slot] != subresource)
		{
			table.UAV[slot] = resource;
			table.UAV_index[slot] = subresource;
			table.dirty_resources = true;
		}
	}
	void GraphicsDevice_DX12::BindUAVs(SHADERSTAGE stage, const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(stage, resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_DX12::UnbindResources(uint32_t slot, uint32_t num, CommandList cmd)
	{
	}
	void GraphicsDevice_DX12::UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd)
	{
	}
	void GraphicsDevice_DX12::BindSampler(SHADERSTAGE stage, const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		assert(slot < GPU_SAMPLER_HEAP_COUNT);
		auto& table = GetFrameResources().descriptors[cmd]->tables[stage];
		if (table.SAM[slot] != sampler)
		{
			table.SAM[slot] = sampler;
			table.dirty_samplers = true;
		}
	}
	void GraphicsDevice_DX12::BindConstantBuffer(SHADERSTAGE stage, const GPUBuffer* buffer, uint32_t slot, CommandList cmd)
	{
		assert(slot < GPU_RESOURCE_HEAP_CBV_COUNT);
		auto& table = GetFrameResources().descriptors[cmd]->tables[stage];
		if (buffer->desc.Usage == USAGE_DYNAMIC || table.CBV[slot] != buffer)
		{
			table.CBV[slot] = buffer;
			table.dirty_resources = true;
		}
	}
	void GraphicsDevice_DX12::BindVertexBuffers(const GPUBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd)
	{
		assert(count <= 8);
		D3D12_VERTEX_BUFFER_VIEW res[8] = { 0 };
		for (uint32_t i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] != nullptr)
			{
				res[i].BufferLocation = ((ID3D12Resource*)vertexBuffers[i]->resource)->GetGPUVirtualAddress();
				res[i].SizeInBytes = vertexBuffers[i]->desc.ByteWidth;
				if (offsets != nullptr)
				{
					res[i].BufferLocation += (D3D12_GPU_VIRTUAL_ADDRESS)offsets[i];
					res[i].SizeInBytes -= offsets[i];
				}
				res[i].StrideInBytes = strides[i];
			}
		}
		GetDirectCommandList(cmd)->IASetVertexBuffers(static_cast<uint32_t>(slot), static_cast<uint32_t>(count), res);
	}
	void GraphicsDevice_DX12::BindIndexBuffer(const GPUBuffer* indexBuffer, const INDEXBUFFER_FORMAT format, uint32_t offset, CommandList cmd)
	{
		D3D12_INDEX_BUFFER_VIEW res = {};
		if (indexBuffer != nullptr)
		{
			res.BufferLocation = ((ID3D12Resource*)indexBuffer->resource)->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
			res.Format = (format == INDEXBUFFER_FORMAT::INDEXFORMAT_16BIT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
			res.SizeInBytes = indexBuffer->desc.ByteWidth;
		}
		GetDirectCommandList(cmd)->IASetIndexBuffer(&res);
	}
	void GraphicsDevice_DX12::BindStencilRef(uint32_t value, CommandList cmd)
	{
		GetDirectCommandList(cmd)->OMSetStencilRef(value);
	}
	void GraphicsDevice_DX12::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		const float blendFactor[4] = { r, g, b, a };
		GetDirectCommandList(cmd)->OMSetBlendFactor(blendFactor);
	}
	void GraphicsDevice_DX12::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		size_t pipeline_hash = 0;
		wiHelper::hash_combine(pipeline_hash, pso->hash);
		if (active_renderpass[cmd] != nullptr)
		{
			wiHelper::hash_combine(pipeline_hash, active_renderpass[cmd]->hash);
		}
		if (prev_pipeline_hash[cmd] == pipeline_hash)
		{
			return;
		}
		prev_pipeline_hash[cmd] = pipeline_hash;

		ID3D12PipelineState* pipeline = nullptr;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end())
		{
			for (auto& x : pipelines_worker[cmd])
			{
				if (pipeline_hash == x.first)
				{
					pipeline = x.second;
					break;
				}
			}

			if (pipeline == nullptr)
			{
				D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

				if (pso->desc.vs != nullptr)
				{
					desc.VS.pShaderBytecode = pso->desc.vs->code.data;
					desc.VS.BytecodeLength = pso->desc.vs->code.size;
				}
				if (pso->desc.hs != nullptr)
				{
					desc.HS.pShaderBytecode = pso->desc.hs->code.data;
					desc.HS.BytecodeLength = pso->desc.hs->code.size;
				}
				if (pso->desc.ds != nullptr)
				{
					desc.DS.pShaderBytecode = pso->desc.ds->code.data;
					desc.DS.BytecodeLength = pso->desc.ds->code.size;
				}
				if (pso->desc.gs != nullptr)
				{
					desc.GS.pShaderBytecode = pso->desc.gs->code.data;
					desc.GS.BytecodeLength = pso->desc.gs->code.size;
				}
				if (pso->desc.ps != nullptr)
				{
					desc.PS.BytecodeLength = pso->desc.ps->code.size;
					desc.PS.pShaderBytecode = pso->desc.ps->code.data;
				}

				RasterizerStateDesc pRasterizerStateDesc = pso->desc.rs != nullptr ? pso->desc.rs->GetDesc() : RasterizerStateDesc();
				desc.RasterizerState.FillMode = _ConvertFillMode(pRasterizerStateDesc.FillMode);
				desc.RasterizerState.CullMode = _ConvertCullMode(pRasterizerStateDesc.CullMode);
				desc.RasterizerState.FrontCounterClockwise = pRasterizerStateDesc.FrontCounterClockwise;
				desc.RasterizerState.DepthBias = pRasterizerStateDesc.DepthBias;
				desc.RasterizerState.DepthBiasClamp = pRasterizerStateDesc.DepthBiasClamp;
				desc.RasterizerState.SlopeScaledDepthBias = pRasterizerStateDesc.SlopeScaledDepthBias;
				desc.RasterizerState.DepthClipEnable = pRasterizerStateDesc.DepthClipEnable;
				desc.RasterizerState.MultisampleEnable = pRasterizerStateDesc.MultisampleEnable;
				desc.RasterizerState.AntialiasedLineEnable = pRasterizerStateDesc.AntialiasedLineEnable;
				desc.RasterizerState.ConservativeRaster = ((CONSERVATIVE_RASTERIZATION && pRasterizerStateDesc.ConservativeRasterizationEnable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
				desc.RasterizerState.ForcedSampleCount = pRasterizerStateDesc.ForcedSampleCount;


				DepthStencilStateDesc pDepthStencilStateDesc = pso->desc.dss != nullptr ? pso->desc.dss->GetDesc() : DepthStencilStateDesc();
				desc.DepthStencilState.DepthEnable = pDepthStencilStateDesc.DepthEnable;
				desc.DepthStencilState.DepthWriteMask = _ConvertDepthWriteMask(pDepthStencilStateDesc.DepthWriteMask);
				desc.DepthStencilState.DepthFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.DepthFunc);
				desc.DepthStencilState.StencilEnable = pDepthStencilStateDesc.StencilEnable;
				desc.DepthStencilState.StencilReadMask = pDepthStencilStateDesc.StencilReadMask;
				desc.DepthStencilState.StencilWriteMask = pDepthStencilStateDesc.StencilWriteMask;
				desc.DepthStencilState.FrontFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilDepthFailOp);
				desc.DepthStencilState.FrontFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilFailOp);
				desc.DepthStencilState.FrontFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.FrontFace.StencilFunc);
				desc.DepthStencilState.FrontFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.FrontFace.StencilPassOp);
				desc.DepthStencilState.BackFace.StencilDepthFailOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilDepthFailOp);
				desc.DepthStencilState.BackFace.StencilFailOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilFailOp);
				desc.DepthStencilState.BackFace.StencilFunc = _ConvertComparisonFunc(pDepthStencilStateDesc.BackFace.StencilFunc);
				desc.DepthStencilState.BackFace.StencilPassOp = _ConvertStencilOp(pDepthStencilStateDesc.BackFace.StencilPassOp);


				BlendStateDesc pBlendStateDesc = pso->desc.bs != nullptr ? pso->desc.bs->GetDesc() : BlendStateDesc();
				desc.BlendState.AlphaToCoverageEnable = pBlendStateDesc.AlphaToCoverageEnable;
				desc.BlendState.IndependentBlendEnable = pBlendStateDesc.IndependentBlendEnable;
				for (int i = 0; i < 8; ++i)
				{
					desc.BlendState.RenderTarget[i].BlendEnable = pBlendStateDesc.RenderTarget[i].BlendEnable;
					desc.BlendState.RenderTarget[i].SrcBlend = _ConvertBlend(pBlendStateDesc.RenderTarget[i].SrcBlend);
					desc.BlendState.RenderTarget[i].DestBlend = _ConvertBlend(pBlendStateDesc.RenderTarget[i].DestBlend);
					desc.BlendState.RenderTarget[i].BlendOp = _ConvertBlendOp(pBlendStateDesc.RenderTarget[i].BlendOp);
					desc.BlendState.RenderTarget[i].SrcBlendAlpha = _ConvertBlend(pBlendStateDesc.RenderTarget[i].SrcBlendAlpha);
					desc.BlendState.RenderTarget[i].DestBlendAlpha = _ConvertBlend(pBlendStateDesc.RenderTarget[i].DestBlendAlpha);
					desc.BlendState.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(pBlendStateDesc.RenderTarget[i].BlendOpAlpha);
					desc.BlendState.RenderTarget[i].RenderTargetWriteMask = _ParseColorWriteMask(pBlendStateDesc.RenderTarget[i].RenderTargetWriteMask);
				}

				D3D12_INPUT_ELEMENT_DESC* elements = nullptr;
				if (pso->desc.il != nullptr)
				{
					desc.InputLayout.NumElements = (uint32_t)pso->desc.il->desc.size();
					elements = new D3D12_INPUT_ELEMENT_DESC[desc.InputLayout.NumElements];
					for (uint32_t i = 0; i < desc.InputLayout.NumElements; ++i)
					{
						elements[i].SemanticName = pso->desc.il->desc[i].SemanticName;
						elements[i].SemanticIndex = pso->desc.il->desc[i].SemanticIndex;
						elements[i].Format = _ConvertFormat(pso->desc.il->desc[i].Format);
						elements[i].InputSlot = pso->desc.il->desc[i].InputSlot;
						elements[i].AlignedByteOffset = pso->desc.il->desc[i].AlignedByteOffset;
						if (elements[i].AlignedByteOffset == VertexLayoutDesc::APPEND_ALIGNED_ELEMENT)
							elements[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
						elements[i].InputSlotClass = _ConvertInputClassification(pso->desc.il->desc[i].InputSlotClass);
						elements[i].InstanceDataStepRate = pso->desc.il->desc[i].InstanceDataStepRate;
					}
				}
				desc.InputLayout.pInputElementDescs = elements;

				desc.NumRenderTargets = 0;
				desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				if (active_renderpass[cmd] == nullptr)
				{
					desc.NumRenderTargets = 1;
					desc.RTVFormats[0] = _ConvertFormat(BACKBUFFER_FORMAT);
				}
				else
				{
					for (uint32_t i = 0; i < active_renderpass[cmd]->desc.numAttachments; ++i)
					{
						const RenderPassAttachment& attachment = active_renderpass[cmd]->desc.attachments[i];

						switch (attachment.type)
						{
						case RenderPassAttachment::RENDERTARGET:
							switch (attachment.texture->desc.Format)
							{
							case FORMAT_R16_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R16_UNORM;
								break;
							case FORMAT_R32_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT;
								break;
							case FORMAT_R24G8_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
								break;
							case FORMAT_R32G8X24_TYPELESS:
								desc.RTVFormats[desc.NumRenderTargets] = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
								break;
							default:
								desc.RTVFormats[desc.NumRenderTargets] = _ConvertFormat(attachment.texture->desc.Format);
								break;
							}
							desc.NumRenderTargets++;
							break;
						case RenderPassAttachment::DEPTH_STENCIL:
							switch (attachment.texture->desc.Format)
							{
							case FORMAT_R16_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D16_UNORM;
								break;
							case FORMAT_R32_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
								break;
							case FORMAT_R24G8_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
								break;
							case FORMAT_R32G8X24_TYPELESS:
								desc.DSVFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
								break;
							default:
								desc.DSVFormat = _ConvertFormat(attachment.texture->desc.Format);
								break;
							}
							break;
						default:
							assert(0);
							break;
						}

						desc.SampleDesc.Count = attachment.texture->desc.SampleCount;
						desc.SampleDesc.Quality = 0;
					}
				}
				desc.SampleMask = pso->desc.sampleMask;

				switch (pso->desc.pt)
				{
				case POINTLIST:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
					break;
				case LINELIST:
				case LINESTRIP:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
					break;
				case TRIANGLELIST:
				case TRIANGLESTRIP:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					break;
				case PATCHLIST:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
					break;
				default:
					desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
					break;
				}

				desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

				desc.pRootSignature = graphicsRootSig;

				HRESULT hr = device->CreateGraphicsPipelineState(&desc, __uuidof(ID3D12PipelineState), (void**)&pipeline);
				assert(SUCCEEDED(hr));

				SAFE_DELETE_ARRAY(elements);

				pipelines_worker[cmd].push_back(std::make_pair(pipeline_hash, pipeline));
			}
		}
		else
		{
			pipeline = it->second;
		}
		assert(pipeline != nullptr);

		GetDirectCommandList(cmd)->SetPipelineState(pipeline);

		if (prev_pt[cmd] != pso->desc.pt)
		{
			prev_pt[cmd] = pso->desc.pt;

			D3D12_PRIMITIVE_TOPOLOGY d3dType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			switch (pso->desc.pt)
			{
			case TRIANGLELIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				break;
			case TRIANGLESTRIP:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
				break;
			case POINTLIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
				break;
			case LINELIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
				break;
			case LINESTRIP:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
				break;
			case PATCHLIST:
				d3dType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
				break;
			default:
				break;
			};
			GetDirectCommandList(cmd)->IASetPrimitiveTopology(d3dType);
		}
	}
	void GraphicsDevice_DX12::BindComputeShader(const ComputeShader* cs, CommandList cmd)
	{
		prev_pipeline_hash[cmd] = 0; // note: same bind point for compute and graphics in dx12!
		GetDirectCommandList(cmd)->SetPipelineState((ID3D12PipelineState*)cs->resource);
	}
	void GraphicsDevice_DX12::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_DX12::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_DX12::DrawInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->ExecuteIndirect(drawInstancedIndirectCommandSignature, 1, (ID3D12Resource*)args->resource, args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature, 1, (ID3D12Resource*)args->resource, args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_DX12::DispatchIndirect(const GPUBuffer* args, uint32_t args_offset, CommandList cmd)
	{
		GetFrameResources().descriptors[cmd]->validate(cmd);
		GetDirectCommandList(cmd)->ExecuteIndirect(dispatchIndirectCommandSignature, 1, (ID3D12Resource*)args->resource, args_offset, nullptr, 0);
	}
	void GraphicsDevice_DX12::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		GetDirectCommandList(cmd)->CopyResource((ID3D12Resource*)pDst->resource, (ID3D12Resource*)pSrc->resource);
	}
	void GraphicsDevice_DX12::CopyTexture2D_Region(const Texture* pDst, uint32_t dstMip, uint32_t dstX, uint32_t dstY, const Texture* pSrc, uint32_t srcMip, CommandList cmd)
	{
		D3D12_RESOURCE_DESC dst_desc = ((ID3D12Resource*)pDst->resource)->GetDesc();
		D3D12_RESOURCE_DESC src_desc = ((ID3D12Resource*)pSrc->resource)->GetDesc();

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = (ID3D12Resource*)pDst->resource;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = D3D12CalcSubresource(dstMip, 0, 0, dst_desc.MipLevels, dst_desc.DepthOrArraySize);

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = (ID3D12Resource*)pSrc->resource;
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = D3D12CalcSubresource(srcMip, 0, 0, src_desc.MipLevels, src_desc.DepthOrArraySize);


		GetDirectCommandList(cmd)->CopyTextureRegion(&dst, dstX, dstY, 0, &src, nullptr);
	}
	void GraphicsDevice_DX12::MSAAResolve(const Texture* pDst, const Texture* pSrc, CommandList cmd)
	{
	}
	void GraphicsDevice_DX12::UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, int dataSize)
	{
		// This will fully update the buffer on the GPU timeline
		//	But on the CPU side we need to keep the in flight data versioned, and we use the temporary buffer for that
		//	This is like a USAGE_DEFAULT buffer updater on DX11
		// TODO: USAGE_DYNAMIC would not use GPU copies, but then it would need to handle assigning descriptor pointer dynamically. 
		//	However, now descriptors are created ahead of time in CreateBuffer

		assert(buffer->desc.Usage != USAGE_IMMUTABLE && "Cannot update IMMUTABLE GPUBuffer!");
		assert((int)buffer->desc.ByteWidth >= dataSize || dataSize < 0 && "Data size is too big!");

		if (dataSize == 0)
		{
			return;
		}

		dataSize = std::min((int)buffer->desc.ByteWidth, dataSize);
		dataSize = (dataSize >= 0 ? dataSize : buffer->desc.ByteWidth);

		if (buffer->desc.Usage == USAGE_DYNAMIC && buffer->desc.BindFlags & BIND_CONSTANT_BUFFER)
		{
			// Dynamic buffer will be used from host memory directly:
			DynamicResourceState& state = dynamic_constantbuffers[cmd][buffer];
			state.allocation = AllocateGPU(dataSize, cmd);
			memcpy(state.allocation.data, data, dataSize);

			for (int stage = 0; stage < SHADERSTAGE_COUNT; ++stage)
			{
				if (state.binding[stage])
				{
					GetFrameResources().descriptors[cmd]->tables[stage].dirty_resources = true;
				}
			}
		}
		else
		{
			// Contents will be transferred to device memory:

			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = (ID3D12Resource*)buffer->resource;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			if (buffer->desc.BindFlags & BIND_CONSTANT_BUFFER || buffer->desc.BindFlags & BIND_VERTEX_BUFFER)
			{
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			}
			else if (buffer->desc.BindFlags & BIND_INDEX_BUFFER)
			{
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			}
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);

			uint8_t* dest = GetFrameResources().resourceBuffer[cmd]->allocate(dataSize, 1);
			memcpy(dest, data, dataSize);
			GetDirectCommandList(cmd)->CopyBufferRegion(
				(ID3D12Resource*)buffer->resource, 0,
				(ID3D12Resource*)GetFrameResources().resourceBuffer[cmd]->buffer.resource, GetFrameResources().resourceBuffer[cmd]->calculateOffset(dest),
				dataSize
			);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
			GetDirectCommandList(cmd)->ResourceBarrier(1, &barrier);

		}

	}
	void GraphicsDevice_DX12::QueryBegin(const GPUQuery* query, CommandList cmd)
	{
		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			GetDirectCommandList(cmd)->BeginQuery(querypool_timestamp, D3D12_QUERY_TYPE_TIMESTAMP, (uint32_t)query->resource);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			GetDirectCommandList(cmd)->BeginQuery(querypool_occlusion, D3D12_QUERY_TYPE_BINARY_OCCLUSION, (uint32_t)query->resource);
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			GetDirectCommandList(cmd)->BeginQuery(querypool_occlusion, D3D12_QUERY_TYPE_OCCLUSION, (uint32_t)query->resource);
			break;
		}
	}
	void GraphicsDevice_DX12::QueryEnd(const GPUQuery* query, CommandList cmd)
	{
		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_TIMESTAMP:
			GetDirectCommandList(cmd)->EndQuery(querypool_timestamp, D3D12_QUERY_TYPE_TIMESTAMP, (uint32_t)query->resource);
			GetDirectCommandList(cmd)->ResolveQueryData(querypool_timestamp, D3D12_QUERY_TYPE_TIMESTAMP, (uint32_t)query->resource, 1, querypool_timestamp_readback, (uint64_t)query->resource * sizeof(uint64_t));
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
			GetDirectCommandList(cmd)->EndQuery(querypool_occlusion, D3D12_QUERY_TYPE_BINARY_OCCLUSION, (uint32_t)query->resource);
			GetDirectCommandList(cmd)->ResolveQueryData(querypool_occlusion, D3D12_QUERY_TYPE_BINARY_OCCLUSION, (uint32_t)query->resource, 1, querypool_occlusion_readback, (uint64_t)query->resource * sizeof(uint64_t));
			break;
		case GPU_QUERY_TYPE_OCCLUSION:
			GetDirectCommandList(cmd)->EndQuery(querypool_occlusion, D3D12_QUERY_TYPE_OCCLUSION, (uint32_t)query->resource);
			GetDirectCommandList(cmd)->ResolveQueryData(querypool_occlusion, D3D12_QUERY_TYPE_OCCLUSION, (uint32_t)query->resource, 1, querypool_occlusion_readback, (uint64_t)query->resource * sizeof(uint64_t));
			break;
		}
	}
	bool GraphicsDevice_DX12::QueryRead(const GPUQuery* query, GPUQueryResult* result)
	{
		D3D12_RANGE range;
		range.Begin = (size_t)query->resource * sizeof(size_t);
		range.End = range.Begin + sizeof(uint64_t);
		D3D12_RANGE nullrange = {};
		void* data = nullptr;

		switch (query->desc.Type)
		{
		case GPU_QUERY_TYPE_EVENT:
			assert(0); // not implemented yet
			break;
		case GPU_QUERY_TYPE_TIMESTAMP:
			querypool_timestamp_readback->Map(0, &range, &data);
			result->result_timestamp = *(uint64_t*)((size_t)data + range.Begin);
			querypool_timestamp_readback->Unmap(0, &nullrange);
			break;
		case GPU_QUERY_TYPE_TIMESTAMP_DISJOINT:
			directQueue->GetTimestampFrequency(&result->result_timestamp_frequency);
			break;
		case GPU_QUERY_TYPE_OCCLUSION_PREDICATE:
		{
			BOOL passed = FALSE;
			querypool_occlusion_readback->Map(0, &range, &data);
			passed = *(BOOL*)((size_t)data + range.Begin);
			querypool_occlusion_readback->Unmap(0, &nullrange);
			result->result_passed_sample_count = (uint64_t)passed;
			break;
		}
		case GPU_QUERY_TYPE_OCCLUSION:
			querypool_occlusion_readback->Map(0, &range, &data);
			result->result_passed_sample_count = *(uint64_t*)((size_t)data + range.Begin);
			querypool_occlusion_readback->Unmap(0, &nullrange);
			break;
		}

		return true;
	}
	void GraphicsDevice_DX12::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		D3D12_RESOURCE_BARRIER barrierdescs[8];

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];
			D3D12_RESOURCE_BARRIER& barrierdesc = barrierdescs[i];

			switch (barrier.type)
			{
			default:
			case GPUBarrier::MEMORY_BARRIER:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.UAV.pResource = barrier.memory.resource == nullptr ? nullptr : (ID3D12Resource*)barrier.memory.resource->resource;
			}
			break;
			case GPUBarrier::IMAGE_BARRIER:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = (ID3D12Resource*)barrier.image.texture->resource;
				barrierdesc.Transition.StateBefore = _ConvertImageLayout(barrier.image.layout_before);
				barrierdesc.Transition.StateAfter = _ConvertImageLayout(barrier.image.layout_after);
				barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}
			break;
			case GPUBarrier::BUFFER_BARRIER:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = (ID3D12Resource*)barrier.buffer.buffer->resource;
				barrierdesc.Transition.StateBefore = _ConvertBufferState(barrier.buffer.state_before);
				barrierdesc.Transition.StateAfter = _ConvertBufferState(barrier.buffer.state_after);
				barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}
			break;
			}
		}

		GetDirectCommandList(cmd)->ResourceBarrier(numBarriers, barrierdescs);
	}

	GraphicsDevice::GPUAllocation GraphicsDevice_DX12::AllocateGPU(size_t dataSize, CommandList cmd)
	{
		// This case allocates a CPU write access and GPU read access memory from the temporary buffer
		// The application can write into this, but better to not read from it

		FrameResources::ResourceFrameAllocator& allocator = *GetFrameResources().resourceBuffer[cmd];
		GPUAllocation result;

		if (dataSize == 0)
		{
			return result;
		}

		uint8_t* dest = allocator.allocate(dataSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		assert(dest != nullptr); // todo: this needs to be handled as well

		result.buffer = &allocator.buffer;
		result.offset = (uint32_t)allocator.calculateOffset(dest);
		result.data = (void*)dest;
		return result;
	}

	void GraphicsDevice_DX12::EventBegin(const std::string& name, CommandList cmd)
	{
		PIXBeginEvent(GetDirectCommandList(cmd), 0xFF000000, wstring(name.begin(), name.end()).c_str());
	}
	void GraphicsDevice_DX12::EventEnd(CommandList cmd)
	{
		PIXEndEvent(GetDirectCommandList(cmd));
	}
	void GraphicsDevice_DX12::SetMarker(const std::string& name, CommandList cmd)
	{
		PIXSetMarker(GetDirectCommandList(cmd), 0xFFFF0000, wstring(name.begin(), name.end()).c_str());
	}


}
