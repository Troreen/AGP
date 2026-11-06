#pragma once
#include "Controller.h"
#include <unordered_map>
#include <memory>
#include <iostream>

class ControllerFactory
{
public:

	ControllerFactory();

	static ControllerFactory& GetInstance()
	{
		static ControllerFactory instance;
		return instance;
	}

	std::shared_ptr<Controller> GetController(const std::string aName);

private:
	template <typename T>
	void Register(const std::string& aName);

	std::unordered_map<std::string, std::shared_ptr<Controller>> myControllers;
};

