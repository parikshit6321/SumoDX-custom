//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "../Rendering/GameRenderer.h"
#include <time.h>
#include "../Utilities/DirectXSample.h"
#include "../GameObjects/Cylinder.h"
#include "../GameObjects/BossBlock.h"

using namespace concurrency;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Storage;
using namespace Windows::UI::Core;

//----------------------------------------------------------------------

SumoDX::SumoDX():
    
    m_gameActive(false)
{
    m_topScore.bestRoundTime = 0;
}

//----------------------------------------------------------------------

void SumoDX::Initialize(
    _In_ MoveLookController^ controller,
    _In_ GameRenderer^ renderer
    )
{
    // This method is expected to be called as an asynchronous task.
    // Care should be taken to not call rendering methods on the
    // m_renderer as this would result in the D3D Context being
    // used in multiple threads, which is not allowed.

	bBossModeActivated = false;
	timeElapsed = 0.0f;
	localTimeElapsed = 0.0f;
	timeSinceLastBite = 0.0f;
	fireRate = 1;

    m_controller = controller;
    m_renderer = renderer;

	wave = Wave::Wave1;

    m_renderObjects = std::vector<GameObject^>();
	m_enemies = std::vector<AISumoBlock^>();
	m_bullets = std::vector<Bullet^>();

    m_savedState = ref new PersistentState();
    m_savedState->Initialize(ApplicationData::Current->LocalSettings->Values, "SumoGame");

    m_timer = ref new GameTimer();
	srand(time(NULL));

	// Adjusting the difficulty and number of enemies.
	difficulty = (Difficulty)(1 + rand() % 4);
	numOfEnemies = (30 * (int)difficulty) + rand() % 10;

    // Create a box primitive to represent the player.
    // The box will be used to handle collisions and constrain the player in the world.
	m_player = ref new SumoBlock();
	m_player->Position(XMFLOAT3(0.0f, 0.5f, 0.0f));
	// It is added to the list of render objects so that it appears on screen.
	m_renderObjects.push_back(m_player);

	// Initialize player health and ammo.
	playerHealth = 100;
	playerAmmo = 30;

	//Create the enemy
	createEnemies();

	//tell the player about their new opponent
	changeTarget();

	//floor model
	Cylinder^ cylinder;
	cylinder = ref new Cylinder(XMFLOAT3(0.0f, -1.0f, 0.0f), 150.0f, XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_renderObjects.push_back(cylinder);

    m_camera = ref new Camera;
    m_camera->SetProjParams(XM_PI / 2, 1.0f, 0.01f, 100.0f);
    m_camera->SetViewParams(
		XMFLOAT3(-3.0f, 10.0f, 0.0f),// Eye point in world coordinates.
		XMFLOAT3(0.0f, -5.0f, 0.0f),// Look at point in world coordinates.
		XMFLOAT3 (0.0f, 1.0f, 0.0f)      // The Up vector for the camera.
        );

    m_controller->Pitch(m_camera->Pitch());
    m_controller->Yaw(m_camera->Yaw());

    // Load the top score from disk if it exists.
    LoadHighScore();

    // Load the currentScore for saved state if it exists.
    LoadState();

    m_controller->Active(false);
}

//----------------------------------------------------------------------

void SumoDX::LoadGame()
{
	wave = Wave::Wave1;
	
	if (bBossIsDead)
		bBossIsDead = false;

	timeElapsed = 0.0f;
	timeSinceLastBite = 0.0f;
	fireRate = 1;
	playerHealth = 100;

	difficulty = (Difficulty)(1 + rand() % 4);
	
	//reset player and enemy
	m_player->Position(XMFLOAT3(0.0f, 0.5f, 0.0f));
	
	createEnemies();
	changeTarget();

	//reset camera
	m_camera->SetViewParams(
		XMFLOAT3(-3.0f, 10.0f, 0.0f),// Eye point in world coordinates.
		XMFLOAT3(0.0f, -5.0f, 0.0f),// Look at point in world coordinates.
		XMFLOAT3(0.0f, 1.0f, 0.0f)      // The Up vector for the camera.
		);
	m_camera->setEyePosition(m_player->Position());
	m_controller->Pitch(m_camera->Pitch());
    m_controller->Yaw(m_camera->Yaw());
   
	//reset other things
	m_gameActive = false;
    m_timer->Reset();

	//reset the save game
	SaveState();

	/*Initialize(m_controller, m_renderer);*/
}


//----------------------------------------------------------------------


void SumoDX::StartLevel()
{
    m_timer->Reset();
    m_timer->Start();
	m_gameActive = true;
    m_controller->Active(true);
}

//----------------------------------------------------------------------

void SumoDX::PauseGame()
{
    m_timer->Stop();
    SaveState();
}

//----------------------------------------------------------------------

void SumoDX::ContinueGame()
{
    m_timer->Start();
    m_controller->Active(true);
}

//----------------------------------------------------------------------

GameState SumoDX::RunGame()
{
    // This method is called to execute a single time interval for active game play.
    // It returns the resulting state of game play after the interval has been executed.
    m_timer->Update();

	//update the camera to look where the user has specified.
	m_camera->setEyePosition(m_player->Position());
	m_camera->LookDirection(m_controller->LookDirection());
	m_controller->Pitch(m_camera->Pitch());
	m_controller->Yaw(m_camera->Yaw());

	// run one frame of game play.
	m_player->Velocity(m_controller->Velocity());

	if ((wave != Wave::Wave10) && bBossIsDead)
		bBossIsDead = false;

	UpdateDynamics();

	if (playerHealth <= 0)
	{
		m_topScore.bestRoundTime = m_timer->PlayingTime();
		return GameState::PlayerLost;
	}

	if (abs(XMVectorGetY(XMVector3Length(m_player->VectorPosition()))) > 150.0f)
	{
		m_player->Position(XMFLOAT3(0.0f, 0.0f, 0.0f));
	}

	if (bBossIsDead)
	{
		//player won, save his time if it is a new high score.
		m_topScore.bestRoundTime = m_timer->PlayingTime();
		SaveHighScore();
		bBossIsDead = false;
		return GameState::GameComplete;
	}

	if (m_enemies.size() == 0)
	{
		if (wave != Wave::Wave10)
		{
			wave = (Wave)((int)wave + 1);
			createEnemies();
		}

		else if (wave == Wave::Wave10 && !bBossModeActivated)
		{
			bBossModeActivated = true;
			createBoss();
		}

	}

    return GameState::Active;
}

//----------------------------------------------------------------------

void SumoDX::OnSuspending()
{
   
}

//----------------------------------------------------------------------

void SumoDX::OnResuming()
{
  
}

//----------------------------------------------------------------------

void SumoDX::UpdateDynamics()
{
    float timeTotal = m_timer->PlayingTime();
    float timeFrame = m_timer->DeltaTime();
    
	// If the elapsed time is too long, we slice up the time and handle physics over several
	// smaller time steps to avoid missing collisions.
	float timeLeft = timeFrame;
	float deltaTime;
	while (timeLeft > 0.0f)
	{
		deltaTime = min(timeLeft, GameConstants::Physics::FrameLength);
		timeLeft -= deltaTime;

		// Update the player position.
		m_player->Position(m_player->VectorPosition() + m_player->VectorVelocity() * deltaTime);

		// Update the camera position according to player position.
		m_camera->setEyePosition(m_player->Position());

		// Update bullet positions.
		for (int i = 0; i < m_bullets.size(); i++)
		{
			(m_bullets[i])->move(deltaTime);
			if ((m_bullets[i])->bulletStopped())
			{
				eraseBullet(i);
				m_bullets.erase(m_bullets.begin() + i);
				i--;
			}
		}

		//AI Update
		
		for (int i = 0; i < m_enemies.size(); i++)
		{
			(m_enemies[i])->DetermineAIAction(deltaTime);
		}

		//m_enemy->DetermineAIAction(deltaTime);

		// Check for player height change.
		if ((m_player->getHeight()) != 0.5f)
			m_player->setHeight();

		for (int i = 0; i < m_enemies.size(); i++)
		{
			//Check for enemy height change.
			if (((m_enemies[i])->getHeight()) != 0.5f)
				(m_enemies[i])->setHeight();

			//Check for player/enemy colision
			float xDelta = (m_enemies[i])->Position().x - m_player->Position().x;
			float zDelta = (m_enemies[i])->Position().z - m_player->Position().z;

			//since each of our sumo's is 1 unit wide if we subtract one from their position deltas
			float overlap = sqrt(xDelta * xDelta + zDelta * zDelta) - 1;
			XMVECTOR playerToEnemy = (m_enemies[i])->VectorPosition() - m_player->VectorPosition();


			// then if we get their overlap value as a negative number a contact has occured.
			if (overlap < 0)
			{
				// Decrease player health.
				//playerHealth -= 10;
				playerTakeDamage();

				m_player->Position(m_player->VectorPosition() + playerToEnemy * overlap * 0.5f);
				(m_enemies[i])->Position((m_enemies[i])->VectorPosition() - playerToEnemy * overlap * 0.5f);
			}
		}

		for (int i = 0; i < m_enemies.size(); i++)
		{
			for (int j = 0; j < m_bullets.size(); j++)
			{
				if (distance(((m_bullets[j])->VectorPosition()), ((m_enemies[i])->VectorPosition())) < 0.7f)
				{
					m_bullets[j]->Position(XMFLOAT3(400.0f, 400.0f, 400.0f));
					m_enemies[i]->Position(XMFLOAT3(400.0f, 400.0f, 400.0f));
					m_enemies[i]->die();

					changeTarget();

					eraseBullet(j);
					eraseEnemy(i);

					m_bullets.erase(m_bullets.begin() + j);
					m_enemies.erase(m_enemies.begin() + i);
					numOfEnemies--;
					break;
				}
			}
		}

		if (bBossModeActivated)
		{
			// Collision detection for Boss Mesh.
			for (int j = 0; j < m_bullets.size(); j++)
			{
				if (distance(((m_bullets[j])->VectorPosition()), m_boss->VectorPosition()) < 3.5f)
				{
					m_boss->TakeDamage();

					eraseBullet(j);
					m_bullets.erase(m_bullets.begin() + j);

					if (m_boss->isDead())
					{
						bBossIsDead = true;
						
						for (int i = 0; i < m_renderObjects.size(); i++)
						{
							if (m_renderObjects[i] == m_boss)
							{
								m_boss->Position(XMFLOAT3(400.0f, 400.0f, 400.0f));
								m_renderObjects.erase(m_renderObjects.begin() + i);
								delete m_boss;
							}
						}
						break;
					}
				}
			}

			// Create minion at boss' position.
			if ((m_timer->PlayingTime() - localTimeElapsed) > (float)(10 / (int)difficulty))
			{
				createMinion();
				localTimeElapsed = m_timer->PlayingTime();
			}
		}
	}
}

//----------------------------------------------------------------------

void SumoDX::SaveState()
{
    // Save basic state of the game.
    m_savedState->SaveBool(":GameActive", m_gameActive);
	m_savedState->SaveSingle(":LevelPlayingTime", m_timer->PlayingTime());
	
 }

//----------------------------------------------------------------------

void SumoDX::LoadState()
{
    /*m_gameActive = m_savedState->LoadBool(":GameActive", m_gameActive);

    if (m_gameActive)
    {
		m_timer->PlayingTime(m_savedState->LoadSingle(":LevelPlayingTime", 0.0f));	
    }*/
}

//----------------------------------------------------------------------

void SumoDX::SaveHighScore()
{
	int currentBest = m_savedState->LoadSingle(":HighScore:LevelCompleted", 0);

	if (currentBest==NULL || currentBest > m_topScore.bestRoundTime)
	{
		m_savedState->LoadSingle(":HighScore:LevelCompleted", m_topScore.bestRoundTime);
	}
}

//----------------------------------------------------------------------

void SumoDX::LoadHighScore()
{
	m_topScore.bestRoundTime = m_savedState->LoadSingle(":HighScore:LevelCompleted", 0);
}

//----------------------------------------------------------------------

void SumoDX::changeTarget()
{
	m_player->Target(findNearestEnemy());
}

//----------------------------------------------------------------------

AISumoBlock^ SumoDX::findNearestEnemy()
{
	float minDistance = 400.0f;
	AISumoBlock^ nearestEnemy;

	if (m_enemies.size() != 0)
	{
		// First check for nearest minions.
		minDistance = distance((m_player->VectorPosition()), ((m_enemies[0])->VectorPosition()));
		nearestEnemy = m_enemies[0];

		for (int i = 1; i < m_enemies.size(); i++)
		{
			if (m_enemies[i]->isAlive())
			{
				if (distance((m_player->VectorPosition()), ((m_enemies[i])->VectorPosition())) < minDistance)
				{
					minDistance = distance((m_player->VectorPosition()), ((m_enemies[i])->VectorPosition()));
					nearestEnemy = m_enemies[i];
				}
			}
		}

	}
	// Now check for the boss.
	if (bBossModeActivated)
	{
		if (distance((m_player->VectorPosition()), (m_boss->VectorPosition())) < minDistance)
		{
			nearestEnemy = m_boss;
		}
	}

	return nearestEnemy;
}

//----------------------------------------------------------------------

void SumoDX::playerShoot()
{
	if ((m_timer->PlayingTime() - timeElapsed) > fireRate)
	{
		if ((m_player->Target()) != nullptr && (distance((m_player->VectorPosition()), ((m_player->Target())->VectorPosition()))) > 0.4f)
		{
			m_bullets.push_back(ref new Bullet(m_player->Position(), static_cast<AISumoBlock^>(m_player->Target())));
			m_renderObjects.push_back(m_bullets[m_bullets.size() - 1]);

			m_renderer->getMaterialForBullets(this, m_bullets);
		}

		timeElapsed = m_timer->PlayingTime();
	}
}

//----------------------------------------------------------------------

float SumoDX::distance(DirectX::XMVECTOR vector)
{
	XMVECTOR length = XMVector3Length(vector);

	float distance = 0.0f;
	XMStoreFloat(&distance, length);
	return distance;
}

//----------------------------------------------------------------------

float SumoDX::distance(DirectX::XMVECTOR vector1, DirectX::XMVECTOR vector2)
{
	XMVECTOR vectorSub = XMVectorSubtract(vector1, vector2);
	XMVECTOR length = XMVector3Length(vectorSub);

	float distance = 0.0f;
	XMStoreFloat(&distance, length);
	return distance;
}

//----------------------------------------------------------------------

void SumoDX::eraseBullet(int i)
{
	for (int j = 0; j < m_renderObjects.size(); j++)
	{
		if (m_bullets[i] == dynamic_cast<Bullet^>(m_renderObjects[j]))
		{
			m_renderObjects.erase(m_renderObjects.begin() + j);
			break;
		}
	}
}

//----------------------------------------------------------------------

void SumoDX::eraseEnemy(int i)
{
	for (int j = 0; j < m_renderObjects.size(); j++)
	{
		if (m_enemies[i] == dynamic_cast<AISumoBlock^>(m_renderObjects[j]))
		{
			m_renderObjects.erase(m_renderObjects.begin() + j);
		}
	}
}

//----------------------------------------------------------------------

void SumoDX::createEnemies()
{
	numOfEnemies = (3 * (int)wave * (int)difficulty) + (rand() % 10);

	//Create the enemy
	// The box will be used to handle collisions and constrain the player in the world.
	for (int i = 0; i < numOfEnemies; i++)
	{
		m_enemies.push_back(ref new AISumoBlock(XMFLOAT3((m_player->Position().x) + (10 + (rand() % 10)), 0.5f, (m_player->Position().x) + (10 + (rand() % 10))), SumoDX::m_player, static_cast<GameConstants::Behavior>(rand() % 3)));
	}

	m_renderer->getMaterialForEnemies(this, m_enemies);

	// It is added to the list of render objects so that it appears on screen.
	for (int i = 0; i < numOfEnemies; i++)
	{
		m_renderObjects.push_back(m_enemies[i]);
	}
}

//----------------------------------------------------------------------

void SumoDX::createBoss()
{
	bBossIsDead = false;

	// Initialize boss object for later reference.
	m_boss = ref new BossBlock(XMFLOAT3((m_player->Position().x) + 10, 2.5f, (m_player->Position().z) + 10), SumoDX::m_player, static_cast<GameConstants::Behavior>(rand() % 3));

	m_renderer->getMaterialForBoss(this, m_boss);
	m_renderObjects.push_back(m_boss);
	
}

//----------------------------------------------------------------------

void SumoDX::createMinion()
{
	numOfEnemies++;

	m_enemies.push_back(ref new AISumoBlock(XMFLOAT3((m_boss->Position()).x, 0.5f, (m_boss->Position()).z), SumoDX::m_player, static_cast<GameConstants::Behavior>(rand() % 3)));
	m_renderer->getMaterialForEnemies(this, m_enemies);
	m_renderObjects.push_back(m_enemies[(m_enemies.size()) - 1]);
}

//----------------------------------------------------------------------

void SumoDX::playerTakeDamage()
{
	if ( ((m_timer->PlayingTime()) - timeSinceLastBite) > (5 / (int)difficulty) || timeSinceLastBite == 0.0f)
	{
		playerHealth -= 10;
		timeSinceLastBite = m_timer->PlayingTime();
	}
}

//----------------------------------------------------------------------