#pragma once

#include "ServiceLocator.h"
#include "CoroutineHandler.h"
#include "Scheduler.h"

CoroutineIdentification CoroutineHandler::StartCoroutine(Coroutine aCoroutine)
{
	CoroutineIdentification id = myNextId++;
	ServiceLocator::GetInstance().GetScheduler().ResumeNextFrame(aCoroutine.handle);
	myCoroutines.emplace_back(id, std::move(aCoroutine));

	return id;
}

void CoroutineHandler::StopCoroutine(CoroutineIdentification aId)
{
	auto it = std::find_if(myCoroutines.begin(), myCoroutines.end(),
		[aId](const Entry& entry) { return entry.id == aId; });

	if (it != myCoroutines.end())
	{
		//good practice to clear the scheduler of all references to stopped handles? as they are stored there and will be calle even if bla blah du fattar...
		ServiceLocator::GetInstance().GetScheduler().DeleteTimer(it->coroutine.handle); //also remove timers as its called from there even if removed from here problembarnet
		it->coroutine.handle.destroy();
		myCoroutines.erase(it);
	}
}

const bool CoroutineHandler::IsDone(CoroutineIdentification aId) const
{
	for (const CoroutineHandler::Entry& entries : myCoroutines)
	{
		if (entries.id == aId && !entries.coroutine.handle.done())
		{
			return false;
		}
	}

	return true;
}

void CoroutineHandler::Update()
{
	myCoroutines.erase(
		std::remove_if(myCoroutines.begin(), myCoroutines.end(),
			[](const Entry& entry) { return entry.coroutine.handle.done(); }),
		myCoroutines.end()
	);
}
