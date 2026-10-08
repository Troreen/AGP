#pragma once

#include "Coroutine.h"

#include <chrono>

Coroutine Coroutine::promise_type::get_return_object()
{
	return Coroutine
	{
		std::coroutine_handle<promise_type>::from_promise(*this)
	};
}

std::suspend_always Coroutine::promise_type::initial_suspend() noexcept
{
	return {};
}

std::suspend_always Coroutine::promise_type::final_suspend() noexcept
{
	done.release();
	return {};
}

void Coroutine::promise_type::return_void() noexcept
{
}

void Coroutine::promise_type::unhandled_exception()
{
	std::terminate();
}

void Coroutine::Wait() const
{
	handle.promise().done.acquire();
	handle.destroy();
}
