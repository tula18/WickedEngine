#include "RenderPath3D_Deferred.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphics;


void RenderPath3D_Deferred::ResizeBuffers()
{
	RenderPath3D::ResizeBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();

	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;

		desc.Format = wiRenderer::RTFormat_gbuffer_0;
		device->CreateTexture(&desc, nullptr, &rtGBuffer[0]);
		device->SetName(&rtGBuffer[0], "rtGBuffer[0]");

		desc.Format = wiRenderer::RTFormat_gbuffer_1;
		device->CreateTexture(&desc, nullptr, &rtGBuffer[1]);
		device->SetName(&rtGBuffer[1], "rtGBuffer[1]");

		desc.Format = wiRenderer::RTFormat_gbuffer_2;
		device->CreateTexture(&desc, nullptr, &rtGBuffer[2]);
		device->SetName(&rtGBuffer[2], "rtGBuffer[2]");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_hdr;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtDeferred);
		device->SetName(&rtDeferred, "rtDeferred");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.Format = wiRenderer::RTFormat_deferred_lightbuffer;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &lightbuffer_diffuse);
		device->SetName(&lightbuffer_diffuse, "lightbuffer_diffuse");
		device->CreateTexture(&desc, nullptr, &lightbuffer_specular);
		device->SetName(&lightbuffer_specular, "lightbuffer_specular");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = wiRenderer::RTFormat_deferred_lightbuffer;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture(&desc, nullptr, &rtSSS[0]);
		device->SetName(&rtSSS[0], "rtSSS[0]");
		device->CreateTexture(&desc, nullptr, &rtSSS[1]);
		device->SetName(&rtSSS[1], "rtSSS[1]");
	}

	{
		RenderPassDesc desc;
		desc.numAttachments = 6;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_DONTCARE,&rtGBuffer[0],-1 };
		desc.attachments[1] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_CLEAR,&rtGBuffer[1],-1 };
		desc.attachments[2] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_DONTCARE,&rtGBuffer[2],-1 };
		desc.attachments[3] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_DONTCARE,&lightbuffer_diffuse,-1 };
		desc.attachments[4] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_DONTCARE,&lightbuffer_specular,-1 };
		desc.attachments[5] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_CLEAR,&depthBuffer,-1 };

		device->CreateRenderPass(&desc, &renderpass_gbuffer);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 3;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&lightbuffer_diffuse,-1 };
		desc.attachments[1] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&lightbuffer_specular,-1 };
		desc.attachments[2] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1 };

		device->CreateRenderPass(&desc, &renderpass_lights);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&rtGBuffer[0],-1 };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1 };

		device->CreateRenderPass(&desc, &renderpass_decals);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&rtDeferred,-1 };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1 };

		device->CreateRenderPass(&desc, &renderpass_deferredcomposition);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1 };

		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&lightbuffer_diffuse,-1 };
		device->CreateRenderPass(&desc, &renderpass_SSS[0]);

		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&rtSSS[0],-1 };
		device->CreateRenderPass(&desc, &renderpass_SSS[1]);

		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&rtSSS[1],-1 };
		device->CreateRenderPass(&desc, &renderpass_SSS[2]);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 2;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&rtDeferred,-1 };
		desc.attachments[1] = { RenderPassAttachment::DEPTH_STENCIL,RenderPassAttachment::LOADOP_LOAD,&depthBuffer,-1,RenderPassAttachment::STOREOP_DONTCARE };

		device->CreateRenderPass(&desc, &renderpass_transparent);
	}
	{
		RenderPassDesc desc;
		desc.numAttachments = 1;
		desc.attachments[0] = { RenderPassAttachment::RENDERTARGET,RenderPassAttachment::LOADOP_LOAD,&rtDeferred,-1 };

		device->CreateRenderPass(&desc, &renderpass_bloom);
	}
}

void RenderPath3D_Deferred::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	wiJobSystem::context ctx;
	CommandList cmd;

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd] { RenderFrameSetUp(cmd); });
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd] { RenderShadows(cmd); });
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, cmd] { RenderReflections(cmd); });

	// Main scene:
	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);

		device->Barrier(&GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY, IMAGE_LAYOUT_DEPTHSTENCIL), 1, cmd);

		{
			auto range = wiProfiler::BeginRangeGPU("Opaque Scene", cmd);

			device->RenderPassBegin(&renderpass_gbuffer, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getReflectionsEnabled() ? &rtReflection : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_REFLECTION, cmd);
			wiRenderer::DrawScene(wiRenderer::GetCamera(), getTessellationEnabled(), cmd, RENDERPASS_DEFERRED, true, true);

			device->RenderPassEnd(cmd);

			wiProfiler::EndRange(range); // Opaque Scene
		}

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_DEPTHSTENCIL, IMAGE_LAYOUT_COPY_SRC),
				GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_SHADER_RESOURCE, IMAGE_LAYOUT_COPY_DST)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->CopyResource(&depthBuffer_Copy, &depthBuffer, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&depthBuffer, IMAGE_LAYOUT_COPY_SRC, IMAGE_LAYOUT_DEPTHSTENCIL_READONLY),
				GPUBarrier::Image(&depthBuffer_Copy, IMAGE_LAYOUT_COPY_DST, IMAGE_LAYOUT_SHADER_RESOURCE)
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		RenderLinearDepth(cmd);

		RenderSSAO(cmd);
	});

	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);
		wiRenderer::BindCommonResources(cmd);

		RenderDecals(cmd);

		// Deferred lights:
		{
			device->RenderPassBegin(&renderpass_lights, cmd);

			Viewport vp;
			vp.Width = (float)depthBuffer.GetDesc().Width;
			vp.Height = (float)depthBuffer.GetDesc().Height;
			device->BindViewports(1, &vp, cmd);

			device->BindResource(PS, getSSAOEnabled() ? &rtSSAO[0] : wiTextureHelper::getWhite(), TEXSLOT_RENDERPATH_SSAO, cmd);
			device->BindResource(PS, getSSREnabled() ? &rtSSR : wiTextureHelper::getTransparent(), TEXSLOT_RENDERPATH_SSR, cmd);
			wiRenderer::DrawDeferredLights(wiRenderer::GetCamera(), depthBuffer_Copy, rtGBuffer[0], rtGBuffer[1], rtGBuffer[2], cmd);

			device->RenderPassEnd(cmd);
		}
	});


	cmd = device->BeginCommandList();
	wiJobSystem::Execute(ctx, [this, device, cmd] {

		wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), cmd);
		wiRenderer::BindCommonResources(cmd);

		RenderSSS(cmd);

		RenderDeferredComposition(cmd);

		RenderSSR(rtDeferred, rtGBuffer[1], cmd);

		DownsampleDepthBuffer(cmd);

		RenderLightShafts(cmd);

		RenderVolumetrics(cmd);

		RenderRefractionSource(rtDeferred, cmd);

		RenderTransparents(renderpass_transparent, RENDERPASS_FORWARD, cmd);

		RenderOutline(rtDeferred, cmd);

		TemporalAAResolve(rtDeferred, rtGBuffer[1], cmd);

		RenderBloom(renderpass_bloom, cmd);

		RenderPostprocessChain(rtDeferred, rtGBuffer[1], cmd);

	});

	RenderPath2D::Render();

	wiJobSystem::Wait(ctx);
}

void RenderPath3D_Deferred::RenderSSS(CommandList cmd) const
{
	if (getSSSEnabled())
	{
		wiRenderer::Postprocess_SSS(
			rtLinearDepth,
			rtGBuffer[0],
			renderpass_SSS[0],
			renderpass_SSS[1],
			renderpass_SSS[2],
			cmd
		);
	}
}
void RenderPath3D_Deferred::RenderDecals(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->RenderPassBegin(&renderpass_decals, cmd);

	Viewport vp;
	vp.Width = (float)depthBuffer.GetDesc().Width;
	vp.Height = (float)depthBuffer.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	wiRenderer::DrawDeferredDecals(wiRenderer::GetCamera(), depthBuffer_Copy, cmd);

	device->RenderPassEnd(cmd);
}
void RenderPath3D_Deferred::RenderDeferredComposition(CommandList cmd) const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->RenderPassBegin(&renderpass_deferredcomposition, cmd);

	Viewport vp;
	vp.Width = (float)depthBuffer.GetDesc().Width;
	vp.Height = (float)depthBuffer.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	wiRenderer::DeferredComposition(
		rtGBuffer[0],
		rtGBuffer[1],
		rtGBuffer[2],
		lightbuffer_diffuse,
		lightbuffer_specular,
		getSSAOEnabled() ? rtSSAO[0] : *wiTextureHelper::getWhite(),
		rtLinearDepth,
		cmd
	);
	wiRenderer::DrawSky(cmd);

	device->RenderPassEnd(cmd);
}
