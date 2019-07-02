#pragma once

#include "SAGameEntity.h"
#include <set>

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//An object that manipulates time; this allows creating time systems based on the true time, but with effects like 
	//time dilation and time stepping and setting timers influenced on those effects
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TimeManager
	{
	public:
		/* Private key only allows friends to call ctor */
		struct PrivateKey { private: friend class TimeSystem; PrivateKey() {}; };
		void update(PrivateKey key, class TimeSystem& timeSystem);

	public:
		inline void setTimeFreeze(bool bInFreezeTime) { bFreezeTime = bInFreezeTime; }
		inline void setFramesToStep(unsigned int frames) { framesToStep_nextFrame = frames; }
		/*Changing time dilation mid-frame is not supported as it would cause havoc and setting order-depend behavior*/
		inline void setTimeDilationFactor_OnNextFrame(float inTimeDilationFactor) { DilationFactor_nextFrame = inTimeDilationFactor; }

		inline float getDeltaTimeSecs(){return dt_dilatedSecs; }
		inline float getUndilatedTimeSecs(){ return dt_undilatedSecs; }
		inline float getTimeDilationFactor() { return timeDilationFactor; }
		inline int getRemaningFramesToStep() { return framesToStep; }
		inline bool isTimeFrozen() const { return bFreezeTime && framesToStep == 0; }
		inline bool isFrameStepping() { return bFreezeTime && framesToStep > 0; }

	private:
		//next frame pattern prevents affects from happening mid-frame
		float dt_undilatedSecs = 0.f;
		float dt_dilatedSecs = 0.f;

		int framesToStep = 0;
		int framesToStep_nextFrame = 0;
		bool bFreezeTime = false;

		float timeDilationFactor = 1.f;
		float DilationFactor_nextFrame = 1.f;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Time system is a special system, above all systems and is strongly coupled with game base 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TimeSystem
	{
	public:
		inline float getCurrentTime() const { return currentTime; };
		inline float getLastFrameTime() const { return lastFrameTime; };
		inline float getRawDeltaTimeSecs() const { return rawDeltaTimeSecs; };
		inline float getDeltaTimeSecs() const { return deltaTimeSecs; };
		inline float getMAX_DELTA_TIME_SECS() const { return MAX_DELTA_TIME_SECS; };

		/* Private key only allows friends to call ctor*/
		struct PrivateKey { private: friend class GameBase; PrivateKey() {}; };
		void updateTime(PrivateKey);
		void markManagerCritical(PrivateKey, sp<TimeManager>& manager);

	public:
		sp<TimeManager> createManager();
		void destroyManager(sp<TimeManager>& worldTimeManager);

	private:
		float currentTime = 0;
		float lastFrameTime = 0;
		float rawDeltaTimeSecs = 0;
		float deltaTimeSecs = 0.f;
		float MAX_DELTA_TIME_SECS = 0.5f;

		std::set<sp<TimeManager>> managers;
		std::set<sp<TimeManager>> criticalManagers;
	};

}