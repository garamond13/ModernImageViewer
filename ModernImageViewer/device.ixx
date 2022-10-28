module;
#include "framework.h"

export module device;
import config;

export class Device {
public:
	Device()
	{
#ifdef NDEBUG
		constexpr UINT flags{};
#else
		constexpr UINT flags{ D3D11_CREATE_DEVICE_DEBUG };
#endif
		constexpr std::array<D3D_FEATURE_LEVEL, 2> feature_levels{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
		};
		D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, feature_levels.data(), feature_levels.size(), D3D11_SDK_VERSION, device.ReleaseAndGetAddressOf(), nullptr, device_context.ReleaseAndGetAddressOf());
	}
protected:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
};