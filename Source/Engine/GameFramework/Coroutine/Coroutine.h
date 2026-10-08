#pragma once

#include <coroutine>
#include <semaphore>

struct Coroutine
{
    struct promise_type
    {
        Coroutine get_return_object();

        std::suspend_always initial_suspend() noexcept;
        std::suspend_always final_suspend() noexcept;

        void return_void() noexcept;

        void unhandled_exception();

	public:
		std::binary_semaphore done{ 0 };
	};

    void Wait() const;

public:
	std::coroutine_handle<promise_type> handle;
};
