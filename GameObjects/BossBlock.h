#pragma once

#include "AISumoBlock.h"

ref class BossBlock : public AISumoBlock
{
internal:
	BossBlock(DirectX::XMFLOAT3 position, SumoBlock^ target, GameConstants::Behavior aiBehavior);
	void TakeDamage()						{ health--; }
	bool isDead();
	uint16 getHealth()						{ return health; }
private:
	uint16									health;
};

