#pragma once
#include "SAGameEntity.h"
#include "..\Tools\RemoveSpecialMemberFunctionUtils.h"


namespace SA
{
	class GameBase;

	/**
		INVARIANT: No subsystem shall attempt to retrieve another subsystem within its constructor; handle that in "initSystem" virtuals
	 */
	class SubsystemBase : public GameEntity, public RemoveCopies, public RemoveMoves
	{
	public:

	private:
		friend GameBase; //driver of virtual functions

		virtual void tick(float deltaSec){};

		/** Called when main game subsystems can safely be accessed; though not all may be initialized */
		virtual void initSystem() {};

		/** Called when game is shuting down for resource releasing; resources that require other subsystems should prefer releasing here rather than the dtor of the subsystem */
		virtual void shutdown() {}
		virtual void handlePostRender() {}
	};
}