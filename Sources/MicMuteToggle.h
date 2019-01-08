#pragma once

enum class MuteBehavior {
	TOGGLE = 0,
	MUTE = 1,
	UNMUTE = 2,
};

bool IsMuted();

void SetMuted(MuteBehavior);