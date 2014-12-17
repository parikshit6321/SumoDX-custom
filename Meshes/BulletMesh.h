#pragma once

// BulletMesh:
// This class derives from MeshObject and creates a ID3D11Buffer of
// vertices and indices to represent a cube-shaped bullet.

#include "MeshObject.h"

ref class BulletMesh : public MeshObject
{
internal:
	BulletMesh(_In_ ID3D11Device *device);
};

