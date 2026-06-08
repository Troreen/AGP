#pragma once

#include <string>

class Actor;

class Component
{
public:
	virtual ~Component() = default;

	virtual void Update(float aDeltaTime);
	virtual void LateUpdate(float aDeltaTime);
	virtual void OnDestroy();
	virtual void OnActiveChanged(bool anIsActive);
	virtual void OnEnabledChanged(bool anIsEnabled);

	const std::string& GetName() const;
	Actor* GetOwner() const;

	bool IsEnabled() const;
	void SetEnabled(bool anIsEnabled);

private:
	void SetOwner(Actor* anOwner);
	void SetName(std::string aName);

	std::string myName;
	Actor* myOwner = nullptr;
	bool myIsEnabled = true;

	friend class Actor;
};
