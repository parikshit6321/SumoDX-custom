#pragma once

#include "GameObject.h"
#include "AISumoBlock.h"

ref class Bullet : public GameObject
{
internal:
	Bullet();
	Bullet(DirectX::XMFLOAT3 position, AISumoBlock^ target);
	void Initialize(DirectX::XMFLOAT3 position, AISumoBlock^ target);
	void Target(AISumoBlock^ target);
	AISumoBlock^ Target();
	void move(float deltaTime);
	bool bulletStopped();
	float distance(DirectX::XMVECTOR vector1, DirectX::XMVECTOR vector2);

private:
	DirectX::XMFLOAT4X4 m_rotationMatrix;
	AISumoBlock^ m_target;
	float m_angle;
	bool bBulletStopped;
};

