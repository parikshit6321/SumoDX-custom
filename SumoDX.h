//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

// SumoDX:
// This is the main game class.  It controls game play logic and game state.
// Some of the key object classes used by SumoDX are:
//     MoveLookController - for handling all input to control player/camera/cursor movement.
//     GameRenderer - for handling all graphics presentation.
//     Camera - for handling view projections.
//     m_renderObjects <GameObject> - is the list of all objects in the scene that may be rendered.

#include "../GameObjects/GameConstants.h"
#include "../GameObjects/Camera.h"
#include "../GameObjects/GameObject.h"
#include "../Utilities/GameTimer.h"
#include "../Input/MoveLookController.h"
#include "../Utilities/PersistentState.h"
#include "../Rendering/GameRenderer.h"
#include "../GameObjects/AISumoBlock.h"
#include "../GameObjects/SumoBlock.h"
#include "../GameObjects/BossBlock.h"
#include "../GameObjects/Bullet.h"
#include "../Utilities/DirectXSample.h"
#include "../Meshes/BulletMesh.h"

ref class DirectXBase;

//--------------------------------------------------------------------------------------

enum class Difficulty
{
	Easy = 1,
	Normal,
	Hard,
	Nightmare,
};

enum class Wave
{
	Wave1 = 1,
	Wave2,
	Wave3,
	Wave4,
	Wave5,
	Wave6,
	Wave7,
	Wave8,
	Wave9,
	Wave10,
};

enum class GameState
{
	Waiting,
	Active,
	PlayerLost,
	GameComplete,
};

typedef struct
{
    Platform::String^ tag;
    float bestRoundTime;
} HighScoreEntry;

typedef std::vector<HighScoreEntry> HighScoreEntries;

//--------------------------------------------------------------------------------------

ref class GameRenderer;

ref class SumoDX
{
internal:
    SumoDX();

    void Initialize(
        _In_ MoveLookController^ controller,
        _In_ GameRenderer^ renderer
        );

    void LoadGame();
  
  
    void StartLevel();
    void PauseGame();
    void ContinueGame();
    GameState RunGame();

    void OnSuspending();
    void OnResuming();

    bool IsActivePlay()                         { return m_timer->Active(); }
    bool GameActive()                           { return m_gameActive; }
    
    HighScoreEntry HighScore()                  { return m_topScore; }
  
    Camera^ GameCamera()                        { return m_camera; }
	std::vector<GameObject^> RenderObjects()    { return m_renderObjects; }

	void changeTarget();
	
	int getPlayerHealth()						{ return playerHealth; }
	int getPlayerAmmo()							{ return playerAmmo; }
	int getBossHealth()							{ return m_boss->getHealth(); }

	void playerTakeDamage();

	Difficulty getDifficulty()					{ return difficulty; }
	void playerShoot();
	uint16 getBulletCount()						{ return m_bullets.size(); }
	std::vector<Bullet^> getBulletVector()		{ return m_bullets; }

	float distance(DirectX::XMVECTOR vector1, DirectX::XMVECTOR vector2);
	float distance(DirectX::XMVECTOR vector1);

	void eraseBullet(int i);
	void eraseEnemy(int i);

	void createEnemies();
	void createBoss();
	void createMinion();

	uint16 getEnemiesLeft()						{ return m_enemies.size(); }
	uint16 getCurrentWave()						{ return (int)wave; }

	bool bossModeActivated()					{ return bBossModeActivated; }
	AISumoBlock^ findNearestEnemy();

private:
    void LoadState();
    void SaveState();
    void SaveHighScore();
    void LoadHighScore();
 
    void UpdateDynamics();

    MoveLookController^                         m_controller;
    GameRenderer^                               m_renderer;
    Camera^                                     m_camera;

    HighScoreEntry                              m_topScore;
    PersistentState^                            m_savedState;

    GameTimer^                                  m_timer;
    bool                                        m_gameActive;
	
    SumoBlock^									m_player;
	AISumoBlock^								m_enemy;
	BossBlock^									m_boss;
	std::vector<GameObject^>                    m_renderObjects;     // List of all objects to be rendered.
	std::vector<AISumoBlock^>					m_enemies;
	std::vector<Bullet^>						m_bullets;
	
	uint16										targetIndex;
	uint16										numOfEnemies;

	uint16										playerHealth;
	uint16										playerAmmo;

	Difficulty									difficulty;
	Wave										wave;
	bool										bBossIsDead;
	bool										bBossModeActivated;

	float64										timeElapsed;		// For shooting.
	float64										localTimeElapsed;	// For creating minions.
	float64										timeSinceLastBite;	// For zombie bites.
	uint16										fireRate;			// For shooting rate.
};

