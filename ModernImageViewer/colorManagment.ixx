module;
#include "framework.h"

export module color_managment;
import config;
import image;
import shared;

namespace {
	constexpr auto lut_size{ 64 }; //must reflect LUT_SIZE in pixel shader
}

export class Color_managment : public Image {
public:
	Color_managment()
	{
		if (shared::config.color_managment & Config::Color_managment::enable)
			initialize_display_profile_handle();
	}

	~Color_managment() noexcept
	{
		cmsCloseProfile(display_profile);
	}

protected:
	void create_3dtexture()
	{
		if (shared::config.color_managment & Config::Color_managment::enable) {
			//prepare the lut
			std::vector<uint16_t> lut;
			lut.reserve(lut_size * lut_size * lut_size * 4); //width * height * depth * channals
			fill_lut(lut.data());
			transform_lut(lut);
			
			//create 3d texture from the lut
			constexpr D3D11_TEXTURE3D_DESC texture3d_desc{
				.Width{ lut_size },
				.Height{ lut_size },
				.Depth{ lut_size },
				.MipLevels{ 1 },
				.Format{ DXGI_FORMAT_R16G16B16A16_UNORM },
				.Usage{ D3D11_USAGE_IMMUTABLE },
				.BindFlags{ D3D11_BIND_SHADER_RESOURCE },
			};
			D3D11_SUBRESOURCE_DATA subresource_data{
				.pSysMem{ lut.data() },
				.SysMemPitch{ lut_size * 8 }, //width * channals * bytes_per_channal
				.SysMemSlicePitch{ lut_size * lut_size * 8 }, //width * height * channals * bytes_per_channal
			};
			Microsoft::WRL::ComPtr<ID3D11Texture3D> texture3d;
			device->CreateTexture3D(&texture3d_desc, &subresource_data, texture3d.ReleaseAndGetAddressOf());
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;
			device->CreateShaderResourceView(texture3d.Get(), nullptr, shader_resource_view.ReleaseAndGetAddressOf());
			device_context->PSSetShaderResources(1u, 1u, shader_resource_view.GetAddressOf());
		}
	}

private:
	auto get_intent() noexcept
	{
		if (shared::config.color_managment & Config::Color_managment::intent_relative_colormetric)
			return INTENT_RELATIVE_COLORIMETRIC;
		else if (shared::config.color_managment & Config::Color_managment::intent_saturation)
			return INTENT_SATURATION;
		else if (shared::config.color_managment & Config::Color_managment::intent_absolute_colormetric)
			return INTENT_ABSOLUTE_COLORIMETRIC;
		else
			return INTENT_PERCEPTUAL;
	}

	auto get_flags() noexcept
	{
		auto flags{ cmsFLAGS_NOCACHE | cmsFLAGS_NOOPTIMIZE | cmsFLAGS_HIGHRESPRECALC };
		if (shared::config.color_managment & Config::Color_managment::blackpointcompensation)
			flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;
		if (shared::config.color_managment & Config::Color_managment::optimise)
			flags ^= cmsFLAGS_NOOPTIMIZE;
		return flags;
	}

	void fill_lut(uint16_t* lut) noexcept
	{
		int i{};
		for (int b{}; b < lut_size; ++b)
			for (int g{}; g < lut_size; ++g)
				for (int r{}; r < lut_size; ++r) {
					lut[i++] = r * 65535 / (lut_size - 1);
					lut[i++] = g * 65535 / (lut_size - 1);
					lut[i++] = b * 65535 / (lut_size - 1);
					i++; //iterate over alpha
				}
	}

	void initialize_display_profile_handle()
	{
		DWORD dword_max_path{ MAX_PATH };
		char path[MAX_PATH];
		auto dc{ GetDC(nullptr) };
		GetICMProfileA(dc, &dword_max_path, path);
		ReleaseDC(nullptr, dc);
		display_profile = cmsOpenProfileFromFile(path, "r");
	}

	void transform_lut(std::vector<uint16_t>& lut)
	{
		if (display_profile) {
			auto image_profile{ get_embended_icc_profile() };
			
			//default to sRGB
			if (!image_profile)
				image_profile = cmsCreate_sRGBProfile();

			auto htransform{ cmsCreateTransform(image_profile, TYPE_RGBA_16, display_profile, TYPE_RGBA_16, get_intent(), get_flags()) };
			cmsCloseProfile(image_profile);
			cmsDoTransform(htransform, lut.data(), lut.data(), lut_size * lut_size * lut_size);
			cmsDeleteTransform(htransform);
		}
	}
	cmsHPROFILE display_profile;
};