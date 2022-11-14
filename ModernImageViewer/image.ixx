module;
#include "framework.h"

export module image;
import device;

export class Image : public Device {
public:
	void read_image(const std::filesystem::path& path)
	{
		image_input = OIIO::ImageInput::open(path);
		width = image_input->spec().width;
		height = image_input->spec().height;
		std::vector<uint8_t> buffer;
		switch (image_input->spec().format.basetype) {
		case OIIO::TypeDesc::UINT8:
			buffer.reserve(image_input->spec().width * image_input->spec().height * 4); //width * height * num_channals * bytes per channal 
			image_input->read_image(0, 0, 0, 3, OIIO::TypeDesc::UINT8, buffer.data(), 4);
			create_texture(buffer, DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(image_input->spec().width * 4));
			break;
		case OIIO::TypeDesc::UINT16:
			buffer.reserve(image_input->spec().width * image_input->spec().height * 8); //width * height * num_channals * bytes per channal 
			image_input->read_image(0, 0, 0, 3, OIIO::TypeDesc::UINT16, buffer.data(), 8);
			create_texture(buffer, DXGI_FORMAT_R16G16B16A16_UNORM, static_cast<UINT>(image_input->spec().width * 8));
			break;
		}
	}

	void create_texture(std::vector<uint8_t>& buffer, DXGI_FORMAT format, UINT sys_mem_pitch)
	{
		D3D11_SUBRESOURCE_DATA subresource_data{
				.pSysMem{ buffer.data() },
				.SysMemPitch{ sys_mem_pitch }, //width * channals * bytes_per_channal
		};
		D3D11_TEXTURE2D_DESC texture2d_desc{
			.Width{ static_cast<UINT>(image_input->spec().width) },
			.Height{ static_cast<UINT>(image_input->spec().height) },
			.MipLevels{ 1 },
			.ArraySize{ 1 },
			.Format{ format },
			.SampleDesc{
				.Count{ 1 },
			},
			.Usage{ D3D11_USAGE_IMMUTABLE },
			.BindFlags{ D3D11_BIND_SHADER_RESOURCE },
		};
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		device->CreateTexture2D(&texture2d_desc, &subresource_data, texture2d.ReleaseAndGetAddressOf());
		device->CreateShaderResourceView(texture2d.Get(), nullptr, shader_resource_view_image.ReleaseAndGetAddressOf());
	}

	cmsHPROFILE get_embended_icc_profile()
	{
		std::vector<uint8_t> icc_profile_buffer(3'000'000); //a 3 MB buffer should be large enough for all cases  
		auto atribute_type{ image_input->spec().getattributetype("ICCProfile") };
		if (atribute_type != OIIO::TypeDesc::UNKNOWN)
			if (image_input->spec().getattribute("ICCProfile", atribute_type, icc_profile_buffer.data()))
				return cmsOpenProfileFromMem(icc_profile_buffer.data(), icc_profile_buffer.size());
		return nullptr;
	}
	int width;
	int height;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view_image;
	std::unique_ptr<OIIO::ImageInput> image_input;
};