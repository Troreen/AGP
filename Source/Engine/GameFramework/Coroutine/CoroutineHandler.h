#pragma once

#include "coroutine.h"

#include <vector>

using CoroutineIdentification = size_t;

class CoroutineHandler
{
public:
    CoroutineIdentification StartCoroutine(Coroutine aCoroutine);
    void StopCoroutine(CoroutineIdentification aId);

    const bool IsDone(CoroutineIdentification aId) const;

    void Update();

private:
    struct Entry
    {
        CoroutineIdentification id;
        Coroutine coroutine;
        bool* pausedFlag = nullptr;
    };

    std::vector<Entry> myCoroutines;
    CoroutineIdentification myNextId = 0;
};
