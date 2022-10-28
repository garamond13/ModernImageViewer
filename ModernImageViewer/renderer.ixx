module;
#include "framework.h"
#include "vertexShader.h"
#include "pixelShader.h"

export module renderer;
import image;
import config;
import color_managment;
import device;
import shared;

//reflects constant buffer cb1 in pixel shader
struct alignas(16) Cbuffer_cb1_data {
	float image_width;
	float image_height;
	BOOL use_color_managment;
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
		
		//pixel shader
		Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
		device->CreatePixelShader(PIXEL_SHADER, sizeof(PIXEL_SHADER), nullptr, pixel_shader.ReleaseAndGetAddressOf());
		device_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

		//sampler
		auto sampler_desc{ CD3D11_SAMPLER_DESC(CD3D11_DEFAULT()) };
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_state;
		device->CreateSamplerState(&sampler_desc, sampler_state.ReleaseAndGetAddressOf());
		device_context->PSSetSamplers(0, 1, sampler_state.GetAddressOf());

		create_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.ReleaseAndGetAddressOf());
		create_swap_chain();
	}

	void draw_frame()
	{
		//initialize clear color with user configured background color
		static float clear_color[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
		clear_color[0] = GetRValue(shared::config.background_color) / 255.0f;
		clear_color[1] = GetGValue(shared::config.background_color) / 255.0f;
		clear_color[2] = GetBValue(shared::config.background_color) / 255.0f;

		device_context->ClearRenderTargetView(render_target_view.Get(), clear_color);
		device_context->OMSetRenderTargets(1, render_target_view.GetAddressOf(), nullptr);
		device_context->Draw(3, 0);
		swap_chain->Present(1, 0);
	}

	void set_image(const wchar_t* path)
	{
		Image::read_image(path);

		//set constant buffer cb1
		cbuffer_cb1_data.image_width = Image::width;
		cbuffer_cb1_data.image_height = Image::height;
		if (shared::config.color_managment & Config::Color_managment::enable)
			cbuffer_cb1_data.use_color_managment = 1;
		else
			cbuffer_cb1_data.use_color_managment = 0;
		update_constant_buffer<Cbuffer_cb1_data>(cbuffer_cb1.Get(), cbuffer_cb1_data);
		device_context->PSSetConstantBuffers(0, 1, cbuffer_cb1.GetAddressOf());

		Color_managment::create_3dtexture();
		set_viewport();
	}

protected:
	void on_window_resize()
	{
		if (swap_chain) {
			device_context->OMSetRenderTargets(0, nullptr, nullptr);
			render_target_view.Reset();
			device_context->Flush();
			swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			create_render_target_view();
			set_viewport();
		}
	}

private:
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
		DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1{
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

		dxgi_factory2->MakeWindowAssociation(shared::hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN);
		create_render_target_view();
		set_viewport();
	}

	void create_render_target_view()
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
		swap_chain->GetBuffer(0u, IID_PPV_ARGS(back_buffer.ReleaseAndGetAddressOf()));
		device->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view.ReleaseAndGetAddressOf());
	}

	void set_viewport()
	{
		D3D11_VIEWPORT viewport{};
		if (Image::width && Image::height) {
			//the swapchain will have a size that matches the window size
			DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
			swap_chain->GetDesc1(&swap_chain_desc1);

			//maximise the image size inside the window but keep the aspect ratio of the image and center it
			if (static_cast<float>(swap_chain_desc1.Width) / static_cast<float>(swap_chain_desc1.Height) > static_cast<float>(Image::width) / static_cast<float>(Image::height)) {
				viewport.Width = static_cast<float>(Image::width) * static_cast<float>(swap_chain_desc1.Height) / static_cast<float>(Image::height);
				viewport.Height = static_cast<float>(swap_chain_desc1.Height);

				//offset image in order to center it in the window
				viewport.TopLeftX = (static_cast<float>(swap_chain_desc1.Width) - viewport.Width) / 2.0f;
			}
			else {
				viewport.Width = static_cast<float>(swap_chain_desc1.Width);
				viewport.Height = static_cast<float>(Image::height) * static_cast<float>(swap_chain_desc1.Width) / static_cast<float>(Image::width);

				//offset image in order to center it in the window
				viewport.TopLeftY = (static_cast<float>(swap_chain_desc1.Height) - viewport.Height) / 2.0f;
			}
		}
		device_context->RSSetViewports(1, &viewport);
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

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target_view;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbuffer_cb1;
	Cbuffer_cb1_data cbuffer_cb1_data;
};