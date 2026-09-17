#pragma once

// KeepLocal retains the local pose; KeepWorld preserves the world pose when the
// new parent's inverse produces a representable local TRS.
enum class ReparentMode
{
	KeepLocal,
	KeepWorld
};
