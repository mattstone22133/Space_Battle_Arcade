#include <iostream>

#include "SAGameBase.h"

#include "SASystemBase.h"
#include "SAWindowSystem.h"
#include "SAAssetSystem.h"
#include "SALevelSystem.h"
#include "SAPlayerSystem.h"
#include "SAParticleSystem.h"
#include "SAAutomatedTestSystem.h"

#include "../Rendering/SAWindow.h"
#include "SALog.h"
#include "SARandomNumberGenerationSystem.h"

namespace SA
{

	GameBase::GameBase()
	{
		//allows subclasses to have local-static singleton getters
		if(!RegisteredSingleton)
		{
			RegisteredSingleton = this;
		}
		else
		{
			throw std::runtime_error("Only a single instance of the game can be created.");
		}

		//initialize time management systems
		systemTimeManager = timeSystem.createManager();
		timeSystem.markManagerCritical(TimeSystem::PrivateKey{}, systemTimeManager);
		
		//create and register systems
		windowSystem = new_sp<WindowSystem>();
		systems.insert(windowSystem);

		assetSystem = new_sp<AssetSystem>();
		systems.insert(assetSystem);

		levelSystem = new_sp<LevelSystem>();
		systems.insert(levelSystem);

		playerSystem = new_sp<PlayerSystem>();
		systems.insert(playerSystem);

		particleSystem = new_sp<ParticleSystem>();
		systems.insert(particleSystem);

		systemRNG = new_sp<RNGSystem>();
		systems.insert(systemRNG);

		automatedTestSystem = new_sp<AutomatedTestSystem>();
		systems.insert(automatedTestSystem);

	}

	GameBase* GameBase::RegisteredSingleton = nullptr;
	SA::GameBase& GameBase::get()
	{
		if (!RegisteredSingleton)
		{
			throw std::runtime_error("GAME BASE NOT CREATED");
		}
		return *RegisteredSingleton;
	}

	void GameBase::start()
	{
		//WARNING: any local objects (eg smart pointers) in this function will have lifetime of game!
		if (!bStarted)
		{
			bStarted = true;

			//initialize systems; this is not done in gamebase ctor because it systemse may call gamebase virtuals
			bCustomSystemRegistrationAllowedTimeWindow = true;
			onRegisterCustomSystem();
			bCustomSystemRegistrationAllowedTimeWindow = false;
			for (const sp<SystemBase>& system : systems)
			{
				system->initSystem();
			}

			{ //prevent permanent window reference via scoped destruction
				sp<Window> window = startUp();
				windowSystem->makeWindowPrimary(window);
			}

			//game loop processes
			onGameloopBeginning.broadcast();
			while (!bExitGame)
			{
				TickGameloop_GameBase();
			}

			//begin shutdown process
			shutDown();

			//shutdown systems after game client has been shutdown
			for (const sp<SystemBase>& system : systems)
			{
				system->shutdown();
			}
		}
	}

	void GameBase::startShutdown()
	{
		log("GameFramework", LogLevel::LOG, "Shutdown Initiated");
		bExitGame = true;
	}

	void GameBase::TickGameloop_GameBase()
	{
		timeSystem.updateTime(TimeSystem::PrivateKey{});
		float deltaTimeSecs = systemTimeManager->getDeltaTimeSecs();

		GameEntity::cleanupPendingDestroy(GameEntity::CleanKey{});

		//#consider having system pass a reference to the system time manager, rather than a float; That way critical systems can ignore manipulation time effects or choose to use time affects. Passing raw time means systems will be forced to use time effects (such as dilation)
		for (const sp<SystemBase>& system : systems) { system->tick(deltaTimeSecs);	}

		//NOTE: there probably needs to be a priority based pre/post loop; but not needed yet so it is not implemented (priorities should probably be defined in a single file via template specliazations)
		PreGameloopTick.broadcast(deltaTimeSecs);
		tickGameLoop(deltaTimeSecs);
		PostGameloopTick.broadcast(deltaTimeSecs);

		renderLoop(deltaTimeSecs); //#future perhaps this should just hook into the OnRenderDispatch below
		onRenderDispatch.broadcast(deltaTimeSecs); //perhaps this needs to be a sorted structure with prioritizes; but that may get hard to maintain. Needs to be a systematic way for UI to come after other rendering.

		//perhaps this should be a subscription service since few systems care about post render
		for (const sp<SystemBase>& system : postRenderNotifys) { system->handlePostRender();}
	}

	void GameBase::subscribePostRender(const sp<SystemBase>& system)
	{
		postRenderNotifys.insert(system);
	}

	void GameBase::RegisterCustomSystem(const sp<SystemBase>& system)
	{
		if (bCustomSystemRegistrationAllowedTimeWindow)
		{
			systems.insert(system);
		}
		else
		{
			std::cerr << "FATAL: attempting to register a custom system outside of start up window; use appropraite virtual function to register these systems" << std::endl;
		}
	}


}


