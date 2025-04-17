#pragma once

#include <IPluginInterface.h>

#include <Glacier/ZHM5CrippleBox.h>
#include <Glacier/SGameUpdateEvent.h>

class ChaosEvents : public IPluginInterface {

public:
	
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
	
	// The debug events may or may not be needed, at least random won't pick them.
	enum class EChaosEvent { 
		DebugSampleFirstEvent, // Do not assign

		// All working effects:
		KillAura,
		ReviveAura,
		InfiniteAmmo,
		Make47Invincible,

		// TODO Effects: As of now, don't work
		RemoveAllWeapons,
		MakeAllNPCsInvisible,
		MakeAllNPCsEnforcers,
		//SpawnFireExtinguishers,
		//SpawnRubberDucks,
		//SpawnRandomExplosions,
		//RandomTeleport,

		DebugSampleLastEvent   // Do not assign
	};

	struct ChaosEventData {
		std::function<void()> effectFunction;
		// The duration of the effect: setting equal to 1 should only run the effect one time. Zero is treated like the effect has expired.
		int effectDuration = 1000;
		// Call to check if effect is active and running
		bool isEffectActive = false;
		// Bool for effects that need something to be ran once
		bool justStarted = true;
	};

private:
	std::unordered_map<EChaosEvent, ChaosEventData> eventHandlers;
	std::unordered_map<EChaosEvent, ChaosEventData> activeEffects;
	EChaosEvent m_CurrentEvent = EChaosEvent::DebugSampleFirstEvent;
	void HandleKillAura();
	void HandleReviveAura();
	void HandleInfiniteAmmo();
	void HandleMake47Invincible();

	void HandleRemoveAllWeapons();
	void HandleMakeAllNPCsInvisible();
	void HandleMakeAllNPCsEnforcers();
	

public:
	void ExecuteEvent(EChaosEvent event);
	EChaosEvent GetRandomEvent();
	void ExecuteRandomEvent();
	void CreateCrippleBox();

	ChaosEvents() {
		eventHandlers = {
			{ EChaosEvent::KillAura, {[this]() { HandleKillAura(); }, 10000} },
			{ EChaosEvent::ReviveAura, {[this]() { HandleReviveAura(); }, 10000} },
			{ EChaosEvent::InfiniteAmmo, {[this]() { HandleInfiniteAmmo(); }, 10000} },
			{ EChaosEvent::Make47Invincible, {[this]() { HandleMake47Invincible(); }, 10000} },

			{ EChaosEvent::RemoveAllWeapons, {[this]() { HandleRemoveAllWeapons(); }, 1} },
			{ EChaosEvent::MakeAllNPCsInvisible, {[this]() { HandleMakeAllNPCsInvisible(); }, 1000} },
			{ EChaosEvent::MakeAllNPCsEnforcers, {[this]() { HandleMakeAllNPCsEnforcers(); }, 1000} }
		};
	}

	ZSceneData* m_LastLoadedScene = nullptr;
	ZEntitySceneContext* m_LastLoadedSceneContext = nullptr;

	ZHM5CrippleBox* hm5CrippleBox = nullptr;
};

/*

Chaos Effect Ideas:
- Spawn rubber ducks
- Fire extinguishers
- Put the targets right next to hitman
- Aimbot?
- Lag the camera behind
- Make the camera go crazy
- Remove all weapons from inventory
- Infinite ammo
- Make all NPCs invisible
- Make all NPCs enforcers

Other Modifier Ideas:
- Base time per is 30 sec
- - Optional: Ability to change time between, maybe no chaos for a time

*/