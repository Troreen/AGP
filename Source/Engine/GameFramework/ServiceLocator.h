#pragma once

namespace CommonUtilities
{
	class InputMapper;
}

class ServiceLocator
{
	public:
		ServiceLocator(const ServiceLocator&) = delete;
		ServiceLocator& operator=(const ServiceLocator&) = delete;

		ServiceLocator(ServiceLocator&&) = delete;
		ServiceLocator& operator=(ServiceLocator&&) = delete;

		static ServiceLocator& GetInstance();

		CommonUtilities::InputMapper* GetInputMapper();
		CommonUtilities::InputMapper* SetInputMapper(CommonUtilities::InputMapper* anInputMapper);

		void KillServices();

	private:
		ServiceLocator();
		~ServiceLocator();

		CommonUtilities::InputMapper* myOwnedInputMapper;
};
