#include "pch.h"
#include "BossBlock.h"


BossBlock::BossBlock(DirectX::XMFLOAT3 position, SumoBlock^ target, GameConstants::Behavior aiBehavior)
: AISumoBlock(position, target, aiBehavior)
{
	health = 10;
	setSpeedFactor(1.0f);
}

bool BossBlock::isDead()
{
	if (health <= 0)
		return true;
	else
		return false;
}
