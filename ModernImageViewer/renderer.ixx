module;
#include "framework.h"
#include "vertexShader.h"
#include "resample.h"
#include "sample.h"
#include "blur.h"

export module renderer;
import image;
import config;
import color_managment;
import device;
import shared;

//reflects constant buffer cb1 in pixel shader
struct alignas(16) Cbuffer_cb1_data {
	
	BOOL use_color_managment;
	float axis_x;
	float axis_y;
	float blur_sigma;
	float blur_radius;
	float unsharp_amount;
};

struct alignas(16) Cbuffer_cb2_data {
	int kernel_index;
	float radius;
	float blur;
	float padding; //not used in shader
	float kparam1;
	float kparam2;
	float antiringing;
	float widening_factor;
};

export class Renderer : public Color_managment {
public:
	void initialize()
	{
		//input layout
		device_context->IASetInputLayout(nullptr);
		device_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//vertex shader
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
		device->CreateVertexShader(VERTEX_SHADER, sizeof(VERTEX_SHADER), nullptr, vertex_shader.ReleaseAndGetAddressOf());
		device_context->VSSetShader(vertex_shader.Get(), nullptr, 0);

		//sampler
		constexpr D3D11_SAMPLER_DESC sampler_desc{
			.Filter{ D3D11_FILTER_MIN_MAG_MIP_LINEAR },
			.AddressU{ D3D11_TEXTURE_ADDRESS_CLAMP },
			.AddressV{ D3D11_TEXTURE_ADDRESS_CLAMP },
			.AddressW{ D3D11_TEXTURE_ADDRESS_CLAMP },
			.MaxAnisotropy{ 1 },
			.ComparisonFunc{ D3D11_COMPARISON_NEVER },
			.MaxLOD{ D3D11_FLOAT32_MAX },
		};
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state;
		device->CreateSamplerState(&sampler_desc, sampler_state.ReleaseAndGetAddressOf());
		device_context->PSSetSamplers(0, 1, sampler_state.GetAddressOf());

		//create constant buffers
		create_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.ReleaseAndGetAddressOf());
		create_constant_buffer<Cbuffer_cb2_data>(cbuffer_cb2.ReleaseAndGetAddressOf());
		
		create_swap_chain();
	}

	void draw_frame()
	{
		if (image_input) {
			//update constant buffer
			cbuffer_cb1_data.use_color_managment = 0;
			cbuffer_cb1_data.axis_x = 0.0;
			cbuffer_cb1_data.axis_y = 1.0;
			cbuffer_cb1_data.blur_sigma = shared::config.blur_sigma;
			cbuffer_cb1_data.blur_radius = shared::config.blur_radius;
			cbuffer_cb1_data.unsharp_amount = 0.0;
			update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);

			draw_pass(Image::width, Image::height, shader_resource_view_image.GetAddressOf(), shader_resource_view_blur_y.ReleaseAndGetAddressOf(), render_target_view_blur_y.ReleaseAndGetAddressOf(), BLUR, sizeof(BLUR));

			//update constant buffer
			cbuffer_cb1_data.use_color_managment = 0;
			cbuffer_cb1_data.axis_x = 1.0;
			cbuffer_cb1_data.axis_y = 0.0;
			cbuffer_cb1_data.blur_sigma = shared::config.blur_sigma;
			cbuffer_cb1_data.blur_radius = shared::config.blur_radius;
			cbuffer_cb1_data.unsharp_amount = 0.0;
			update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);

			draw_pass(Image::width, Image::height, shader_resource_view_blur_y.GetAddressOf(), shader_resource_view_blur_x.ReleaseAndGetAddressOf(), render_target_view_blur_x.ReleaseAndGetAddressOf(), BLUR, sizeof(BLUR));

			//update constant buffer
			cbuffer_cb1_data.use_color_managment = 0;
			cbuffer_cb1_data.axis_x = 0.0;
			cbuffer_cb1_data.axis_y = 1.0;
			update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);

			draw_pass(Image::width, swap_chain_desc1.Height, shader_resource_view_blur_x.GetAddressOf(), shader_resource_view_resample_y.ReleaseAndGetAddressOf(), render_target_view_resample_y.ReleaseAndGetAddressOf(), RESAMPLE, sizeof(RESAMPLE));

			//update constant buffer
			cbuffer_cb1_data.axis_x = 1.0;
			cbuffer_cb1_data.axis_y = 0.0;
			update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);

			draw_pass(swap_chain_desc1.Width, swap_chain_desc1.Height, shader_resource_view_resample_y.GetAddressOf(), shader_resource_view_resample_x.ReleaseAndGetAddressOf(), render_target_view_resample_x.ReleaseAndGetAddressOf(), RESAMPLE, sizeof(RESAMPLE));

			//update constant buffer
			cbuffer_cb1_data.use_color_managment = 0;
			cbuffer_cb1_data.axis_x = 0.0;
			cbuffer_cb1_data.axis_y = 1.0;
			cbuffer_cb1_data.blur_sigma = shared::config.unsharp_sigma;
			cbuffer_cb1_data.blur_radius = shared::config.unsharp_radius;
			cbuffer_cb1_data.unsharp_amount = shared::config.unsharp_amount;
			update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);

			draw_pass(swap_chain_desc1.Width, swap_chain_desc1.Height, shader_resource_view_resample_x.GetAddressOf(), shader_resource_view_unsharp_y.ReleaseAndGetAddressOf(), render_target_view_unsharp_y.ReleaseAndGetAddressOf(), BLUR, sizeof(BLUR));

			//update constant buffer
			cbuffer_cb1_data.use_color_managment = 0;
			cbuffer_cb1_data.axis_x = 1.0;
			cbuffer_cb1_data.axis_y = 0.0;
			cbuffer_cb1_data.blur_sigma = shared::config.unsharp_sigma;
			cbuffer_cb1_data.blur_radius = shared::config.unsharp_radius;
			cbuffer_cb1_data.unsharp_amount = shared::config.unsharp_amount;
			update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);
			device_context->PSSetShaderResources(2, 1, shader_resource_view_resample_x.GetAddressOf());

			draw_pass(swap_chain_desc1.Width, swap_chain_desc1.Height, shader_resource_view_unsharp_y.GetAddressOf(), shader_resource_view_unsharp_x.ReleaseAndGetAddressOf(), render_target_view_unsharp_x.ReleaseAndGetAddressOf(), BLUR, sizeof(BLUR));

			//pixel shader
			Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
			device->CreatePixelShader(SAMPLE, sizeof(SAMPLE), nullptr, pixel_shader.ReleaseAndGetAddressOf());
			device_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

			//initialize clear color with user configured background color
			static float clear_color[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
			clear_color[0] = GetRValue(shared::config.background_color) / 255.0f;
			clear_color[1] = GetGValue(shared::config.background_color) / 255.0f;
			clear_color[2] = GetBValue(shared::config.background_color) / 255.0f;

			device_context->ClearRenderTargetView(render_target_view.Get(), clear_color);

			//bind resources
			device_context->PSSetShaderResources(0, 1, shader_resource_view_unsharp_x.GetAddressOf());
			device_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), nullptr);

			//update constant buffer
			if (shared::config.color_managment & Config::Color_managment::enable)
				cbuffer_cb1_data.use_color_managment = 1;
			else
				cbuffer_cb1_data.use_color_managment = 0;
			update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);

			set_viewport();
			device_context->Draw(3, 0);
			swap_chain->Present(1, 0);
			unbind_resources();
		}
	}

	void set_image(const std::filesystem::path& path)
	{
		Image::read_image(path);

		//set constant buffer cb1
		if (shared::config.color_managment & Config::Color_managment::enable)
			cbuffer_cb1_data.use_color_managment = 1;
		else
			cbuffer_cb1_data.use_color_managment = 0;
		update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);
		device_context->PSSetConstantBuffers(0, 1, cbuffer_cb1.GetAddressOf());

		update_scaling();
		Color_managment::create_3dtexture();
		Image::image_input->close();
	}

protected:
	void on_window_resize()
	{
		if (swap_chain) {
			device_context->OMSetRenderTargets(0, nullptr, nullptr);
			render_target_view.Reset();
			device_context->Flush();
			swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			swap_chain->GetDesc1(&swap_chain_desc1);
			create_render_target_view();
			update_scaling();
		}
	}

	void set_viewport(float width = 0.0f, float height = 0.0f)
	{
		D3D11_VIEWPORT viewport{
			.Width{ width },
			.Height{ height },
		};
		if (!viewport.Width && !viewport.Height && Image::width && Image::height) {
			//maximise the image size inside the window but keep the aspect ratio of the image and center it
			if (get_ratio<float>(swap_chain_desc1.Width, swap_chain_desc1.Height) > get_ratio<float>(Image::width, Image::height)) {
				viewport.Width = static_cast<float>(Image::width) * get_ratio<float>(swap_chain_desc1.Height, Image::height);
				viewport.Height = static_cast<float>(swap_chain_desc1.Height);

				//offset image in order to center it in the window
				viewport.TopLeftX = (static_cast<float>(swap_chain_desc1.Width) - viewport.Width) / 2.0f;
			}
			else {
				viewport.Width = static_cast<float>(swap_chain_desc1.Width);
				viewport.Height = static_cast<float>(Image::height) * get_ratio<float>(swap_chain_desc1.Width, Image::width);

				//offset image in order to center it in the window
				viewport.TopLeftY = (static_cast<float>(swap_chain_desc1.Height) - viewport.Height) / 2.0f;
			}
		}
		device_context->RSSetViewports(1, &viewport);
	}

private:
	void unbind_resources()
	{
		ID3D11ShaderResourceView* shader_resource_view_nulls[1]{};
		device_context->PSSetShaderResources(0, 1, shader_resource_view_nulls);
		ID3D11RenderTargetView* render_target_view_nulls[1]{};
		device_context->OMSetRenderTargets(1, render_target_view_nulls, nullptr);
	}

	void create_swap_chain()
	{
		//query interfaces
		Microsoft::WRL::ComPtr<IDXGIDevice1> dxgi_device2;
		device.As(&dxgi_device2);
		Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter;
		dxgi_device2->GetAdapter(dxgi_adapter.ReleaseAndGetAddressOf());
		Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory2;
		dxgi_adapter->GetParent(IID_PPV_ARGS(dxgi_factory2.ReleaseAndGetAddressOf()));

		//get primary output
		Microsoft::WRL::ComPtr<IDXGIOutput> dxgi_output;
		dxgi_adapter->EnumOutputs(0, dxgi_output.ReleaseAndGetAddressOf());
		
		//get display color bit depth
		Microsoft::WRL::ComPtr<IDXGIOutput6> dxgi_output6;
		dxgi_output.As(&dxgi_output6);
		DXGI_OUTPUT_DESC1 output_desc1;
		dxgi_output6->GetDesc1(&output_desc1);

		//create swap chain
		swap_chain_desc1 = {
			.Format{ output_desc1.BitsPerColor == 10 ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM },
			.SampleDesc{
				.Count{ 1 },
			},
			.BufferUsage{ DXGI_USAGE_RENDER_TARGET_OUTPUT },
			.BufferCount{ 2 },
			.Scaling{ DXGI_SCALING_NONE },
			.SwapEffect{ DXGI_SWAP_EFFECT_FLIP_DISCARD },
		};
		dxgi_factory2->CreateSwapChainForHwnd(dxgi_device2.Get(), shared::hwnd, &swap_chain_desc1, nullptr, nullptr, swap_chain.ReleaseAndGetAddressOf());
		swap_chain->GetDesc1(&swap_chain_desc1);

		dxgi_factory2->MakeWindowAssociation(shared::hwnd, DXGI_MWA_NO_ALT_ENTER);
		create_render_target_view();
	}

	void create_render_target_view()
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
		swap_chain->GetBuffer(0u, IID_PPV_ARGS(back_buffer.ReleaseAndGetAddressOf()));
		device->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view.ReleaseAndGetAddressOf());
	}

	void draw_pass(UINT width, UINT height, ID3D11ShaderResourceView** srv_bind, ID3D11ShaderResourceView** srv, ID3D11RenderTargetView** rtv, const BYTE* shader, size_t shader_size)
	{
		create_pass(width, height, srv, rtv);

		//pixel shader
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
		device->CreatePixelShader(shader, shader_size, nullptr, pixel_shader.ReleaseAndGetAddressOf());
		device_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

		//bind resources
		device_context->PSSetShaderResources(0u, 1, srv_bind);
		device_context->OMSetRenderTargets(1, rtv, nullptr);

		constexpr float color[4]{ 0.0, 0.0, 0.0, 1.0 };
		device_context->ClearRenderTargetView(*rtv, color);
		set_viewport(width, height);
		device_context->Draw(3, 0);
		unbind_resources();
	}

	void create_pass(UINT width, UINT height, ID3D11ShaderResourceView** srv, ID3D11RenderTargetView** rtv)
	{
		//texture
		D3D11_TEXTURE2D_DESC texture2d_desc{
			.Width{ width },
			.Height{ height },
			.MipLevels{ 1 },
			.ArraySize{ 1 },
			.Format{ DXGI_FORMAT_R32G32B32A32_FLOAT },
			.SampleDesc{
				.Count{ 1 },
			},
			.Usage{ D3D11_USAGE_DEFAULT },
			.BindFlags{ D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET },
		};
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		device->CreateTexture2D(&texture2d_desc, nullptr, texture2d.ReleaseAndGetAddressOf());
		
		//render target view
		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc{
			.Format{ DXGI_FORMAT_R32G32B32A32_FLOAT },
			.ViewDimension{ D3D11_RTV_DIMENSION_TEXTURE2D },
		};
		device->CreateRenderTargetView(texture2d.Get(), &render_target_view_desc, rtv);
		
		//shader resource view
		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{
			.Format{ DXGI_FORMAT_R32G32B32A32_FLOAT },
			.ViewDimension{ D3D11_SRV_DIMENSION_TEXTURE2D },
			.Texture2D{
				.MipLevels{ 1 },
			},
		};
		device->CreateShaderResourceView(texture2d.Get(), &shader_resource_view_desc, srv);
	}

	void update_scaling()
	{
		auto scale_factor{ get_scale_factor() };
		cbuffer_cb2_data.kernel_index = scale_factor == 1.0f ? 0 : shared::config.kernel;
		if(shared::config.kernel == Config::Kernel::bc_spline || shared::config.kernel == Config::Kernel::bicubic)
			cbuffer_cb2_data.radius = 2.0f;
		else if(shared::config.kernel == Config::Kernel::nearest_neighbor)
			cbuffer_cb2_data.radius = 1.0f;
		else
			cbuffer_cb2_data.radius = shared::config.radius;
		cbuffer_cb2_data.blur = shared::config.kernel_blur;
		cbuffer_cb2_data.kparam1 = shared::config.param1;
		cbuffer_cb2_data.kparam2 = shared::config.param2;
		cbuffer_cb2_data.antiringing = shared::config.antiringing;
		cbuffer_cb2_data.widening_factor = scale_factor < 1.0f ? 1.0f / scale_factor : 1.0f;
		update_constant_buffer<Cbuffer_cb2_data>(cbuffer_cb2.Get(), cbuffer_cb2_data);
		device_context->PSSetConstantBuffers(1, 1, cbuffer_cb2.GetAddressOf());
	}

	float get_scale_factor()
	{
		auto x_ratio{ get_ratio<float>(swap_chain_desc1.Width, Image::width) };
		auto y_ratio{ get_ratio<float>(swap_chain_desc1.Height, Image::height) };
		if (x_ratio < 1.0f || y_ratio < 1.0f) //downscale
			return std::min(x_ratio, y_ratio);
		else if (x_ratio > 1.0f || y_ratio > 1.0f) //upscale
			return std::max(x_ratio, y_ratio);
		else
			return 1.0f; //no scaling
	}

	template <typename T>
	void create_constant_buffer(ID3D11Buffer** buffer)
	{
		constexpr D3D11_BUFFER_DESC buffer_desc{
			.ByteWidth{ sizeof(T) },
			.Usage{ D3D11_USAGE_DYNAMIC },
			.BindFlags{ D3D11_BIND_CONSTANT_BUFFER },
			.CPUAccessFlags{ D3D11_CPU_ACCESS_WRITE },
		};
		device->CreateBuffer(&buffer_desc, 0, buffer);
	}

	template <typename T>
	void update_constant_buffer(ID3D11Buffer* buffer, T& data)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_subresource;
		device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
		memcpy(mapped_subresource.pData, &data, sizeof(T));
		device_context->Unmap(buffer, 0);
	}

	template<typename T>
	constexpr T get_ratio(auto a, auto b) noexcept
	{
		return static_cast<T>(a) / static_cast<T>(b);
	}

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbuffer_cb1;
	Cbuffer_cb1_data cbuffer_cb1_data;
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbuffer_cb2;
	Cbuffer_cb2_data cbuffer_cb2_data;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_resample_y;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_resample_y;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_resample_x;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_resample_x;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_blur_y;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_blur_y;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_blur_x;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_blur_x;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_unsharp_y;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_unsharp_y;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view_unsharp_x;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_unsharp_x;
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
};