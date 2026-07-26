#pragma once

#if !defined(AGP_RENDERER_HOST_TEST_FAULTS)
#error RendererHostFaultInjection.h is available only to the dedicated fault-injection test build.
#endif

namespace AGP::Testing
{
	enum class RendererHostFault
	{
		None,
		AfterGraphicsInitialize,
		AfterSwapchainResize
	};

	void SetRendererHostFault(RendererHostFault aFault) noexcept;
	bool ConsumeRendererHostFault(RendererHostFault aFault) noexcept;
}
