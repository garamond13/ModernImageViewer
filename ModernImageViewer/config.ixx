module;
#include "framework.h"

export module config;

//bit  decimal_value
// 1   1
// 2   2
// 3   4
// 4   8
// 5   16
// 6   32
// 7   64
// 8   128
// 9   256
// 10  512
// 11  1024
// 12  2048
// 13  4096
// 14  8192
// 15  16384
// 16  32768

export class Config {
public:

	using flag_type = uint16_t;

	struct General {
		static constexpr flag_type auto_hide_cursor{ 1 };
		static constexpr flag_type fixed_window_dimensions{ 2 };
		General() = delete;
	};

	struct Color_managment {
		static constexpr flag_type enable{ 1 };
		static constexpr flag_type blackpointcompensation{ 2 };
		static constexpr flag_type intent_perceptual{ 4 };
		static constexpr flag_type intent_relative_colormetric{ 8 };
		static constexpr flag_type intent_saturation{ 16 };
		static constexpr flag_type intent_absolute_colormetric{ 32 };
		static constexpr flag_type optimise{ 64 }; //trade quality for performance
		Color_managment() = delete;
	};

	struct Kernel {
		//linear == 0;
		static constexpr flag_type lanczos{ 1 };
		static constexpr flag_type cosine{ 2 };
		static constexpr flag_type hann{ 3 };
		static constexpr flag_type hamming{ 4 };
		static constexpr flag_type blackman{ 5 };
		static constexpr flag_type kaiser{ 6 };
		static constexpr flag_type welch{ 7 };
		static constexpr flag_type said{ 8 };
		static constexpr flag_type bc_spline{ 9 }; //fixed radius, 2.0
		static constexpr flag_type bicubic{ 10 }; //fixed radius, 2.0
		static constexpr flag_type nearest_neighbor{ 11 }; //fixed radius, 1.0
		Kernel() = delete;
	};

	void write()
	{
		std::filesystem::path config_path;
		get_path(config_path);

		//open file for writing, than write line by line
		std::wofstream file(config_path);
		file
			<< general << '\n'
			<< color_managment << '\n'
			<< window_width << '\n'
			<< window_height << '\n'
			<< background_color << '\n';
		for (size_t i{}; i < custom_colors.size(); ++i)
			file << custom_colors[i] << '\n';
		file
			<< upscale_kernel << '\n'
			<< upscale_radius << '\n'
			<< upscale_kernel_blur << '\n'
			<< upscale_param1 << '\n'
			<< upscale_param2 << '\n'
			<< antiringing << '\n'
			<< upscale_unsharp_radius << '\n'
			<< upscale_unsharp_sigma << '\n'
			<< upscale_unsharp_amount << '\n'
			<< downscale_kernel << '\n'
			<< downscale_radius << '\n'
			<< downscale_kernel_blur << '\n'
			<< downscale_param1 << '\n'
			<< downscale_param2 << '\n'
			<< blur_radius << '\n'
			<< blur_sigma << '\n'
			<< downscale_unsharp_radius << '\n'
			<< downscale_unsharp_sigma << '\n'
			<< downscale_unsharp_amount << '\n';
	}

	void read()
	{
		std::filesystem::path config_path;
		get_path(config_path);

		//chechs if the config.dat exists and if not creates it with default settings
		if (!std::filesystem::exists(config_path))
			write_defaults();

		//open file for reading, than read line by line
		std::wifstream file(config_path);
		file
			>> general
			>> color_managment
			>> window_width
			>> window_height
			>> background_color;
		for (int i{}; i < custom_colors.size(); ++i)
			file >> custom_colors[i];
		file
			>> upscale_kernel
			>> upscale_radius
			>> upscale_kernel_blur
			>> upscale_param1
			>> upscale_param2
			>> antiringing
			>> upscale_unsharp_radius
			>> upscale_unsharp_sigma
			>> upscale_unsharp_amount
			>> downscale_kernel
			>> downscale_radius
			>> downscale_kernel_blur
			>> downscale_param1
			>> downscale_param2
			>> blur_radius
			>> blur_sigma
			>> downscale_unsharp_radius
			>> downscale_unsharp_sigma
			>> downscale_unsharp_amount;

	}

	//if passed flags are already set it will unset them
	void set_color_managment(uint16_t flags)
	{
		if (color_managment & flags) {
			auto tmp{ color_managment };
			color_managment ^= flags;
			flags ^= tmp;
		}
		color_managment |= flags;
	}

	void set_general(uint16_t flags)
	{
		if (general & flags) {
			auto tmp{ general };
			general ^= flags;
			flags ^= tmp;
		}
		general |= flags;
	}

	void set_defaults() noexcept
	{
		general = General::auto_hide_cursor | General::fixed_window_dimensions;
		color_managment = Color_managment::enable | Color_managment::intent_perceptual;
		window_width = 1200;
		window_height = 900;
		upscale_kernel = Kernel::hann;
		upscale_radius = 3.0;
		upscale_kernel_blur = 1.0;
		upscale_unsharp_radius = 3.0;
		upscale_unsharp_sigma = 1.0;
		upscale_unsharp_amount = 0.5;
		downscale_kernel = Kernel::hann;
		downscale_radius = 3.0;
		downscale_kernel_blur = 1.0;
		blur_radius = 3.0;
		blur_sigma = 1.0;
		downscale_unsharp_radius = 3.0;
		downscale_unsharp_sigma = 1.0;
		downscale_unsharp_amount = 0.5;
	}

	void write_defaults()
	{
		set_defaults();
		write();
	}

	flag_type general;
	flag_type color_managment;
	int window_width;
	int window_height;
	COLORREF background_color;
	std::array<COLORREF, 16> custom_colors;
	flag_type upscale_kernel;
	float upscale_radius;
	float upscale_kernel_blur;
	float upscale_param1;
	float upscale_param2;
	float antiringing;
	float upscale_unsharp_radius;
	float upscale_unsharp_sigma;
	float upscale_unsharp_amount;
	flag_type downscale_kernel;
	float downscale_radius;
	float downscale_kernel_blur;
	float downscale_param1;
	float downscale_param2;
	float blur_radius;
	float blur_sigma;
	float downscale_unsharp_radius;
	float downscale_unsharp_sigma;
	float downscale_unsharp_amount;

	private:
		//config path: %USERPROFILE%\AppData\Local\ModernImageViever\config.dat
		//in debug use local path
		void get_path(std::filesystem::path& path)
		{

#ifdef NDEBUG
			PWSTR local_app_data;
			SHGetKnownFolderPath(FOLDERID_LocalAppData, 0ul, nullptr, &local_app_data);
			path = local_app_data;

			//append %USERPROFILE%\AppData\Local with our directory
			//check first if it exists and if not, create it
			path += L"\\ModernImageViewer";
			if (!std::filesystem::exists(path))
				std::filesystem::create_directory(path);

			//finaly make the full path of the config file
			path += L"\\config.dat";

			CoTaskMemFree(local_app_data);
#else
			path = L"config.dat";
#endif

		}
};