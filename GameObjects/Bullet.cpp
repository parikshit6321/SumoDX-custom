#include "pch.h"
#include "Bullet.h"

using namespace DirectX;

Bullet::Bullet()
{
	Initialize(XMFLOAT3(0, 0, 0), nullptr);
}

Bullet::Bullet(DirectX::XMFLOAT3 position, AISumoBlock^ target)
{
	Initialize(position, target);
}

void Bullet::Initialize(DirectX::XMFLOAT3 position, AISumoBlock^ target)
{
	bBulletStopped = false;
	m_target = target;
	if (target != nullptr)
	{
		m_angle = XMVectorGetY(XMVector3AngleBetweenNormals(XMVector3Normalize(m_target->VectorPosition() - VectorPosition()), XMVector3Normalize(XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f))));
	}
	Position(position);
	XMMATRIX mat1 = XMMatrixIdentity();
	XMStoreFloat4x4(&m_rotationMatrix, mat1);
}


void Bullet::Target(AISumoBlock^ target)
{
	m_target = target;
}

AISumoBlock^ Bullet::Target()
{
	return m_target;
}

void Bullet::move(float deltaTime)
{
	if (m_target != nullptr)
	{
		if (distance((this->VectorPosition()), m_target->VectorPosition()) == 0.0f)
		{
			bBulletStopped = true;
		}

		//Face the target.
		XMVECTOR direction = XMVector3Normalize(m_target->VectorPosition() - VectorPosition());
		float ans = XMVectorGetY(XMVector2AngleBetweenNormals(direction, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f)));
		m_angle = ans;

		//if you are in the upper arc
		if (XMVectorGetZ(direction) > 0)
		{
			m_angle *= -1;
		}

		XMStoreFloat4x4(&m_modelMatrix, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixRotationY(m_angle) * XMMatrixTranslation(m_position.x, m_position.y, m_position.z));

		XMVectorSetIntY(direction, 0);
		Position(VectorPosition() + XMVector3Normalize(direction) * deltaTime * 10);
	}
	else
	{
		XMStoreFloat4x4(&m_modelMatrix, XMMatrixScaling(1.0f, 1.0f, 1.0f) *	XMLoadFloat4x4(&m_rotationMatrix) *	XMMatrixTranslation(m_position.x, m_position.y, m_position.z));
	}


}

bool Bullet::bulletStopped()
{
	return bBulletStopped;
}

float Bullet::distance(DirectX::XMVECTOR vector1, DirectX::XMVECTOR vector2)
{
	XMVECTOR vectorSub = XMVectorSubtract(vector1, vector2);
	XMVECTOR length = XMVector3Length(vectorSub);

	float distance = 0.0f;
	XMStoreFloat(&distance, length);
	return distance;
}