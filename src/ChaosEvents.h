#pragma once

#include <IPluginInterface.h>

#include <Glacier/SGameUpdateEvent.h>

class ChaosEvents : public IPluginInterface {

public:
	
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
	
	// The debug events may or may not be needed, at least random won't pick them.
	enum class EChaosEvent { 
		DebugSampleFirstEvent, // Do not assign
		KillAura,
		ReviveAura,
		LoadRandomMap,
		RemoveAllWeapons,
		InfiniteAmmo,
		MakeAllNPCsInvisible,
		RandomTeleport,
		SpawnFireExtinguishers,
		SpawnRubberDucks,
		DebugSampleLastEvent   // Do not assign
	};

	struct ChaosEventData {
		std::function<void()> effectFunction;
		int effectDuration;
		bool isEffectActive;
	};

private:
	std::unordered_map<EChaosEvent, ChaosEventData> eventHandlers;
	std::unordered_map<EChaosEvent, ChaosEventData> activeEffects;
	EChaosEvent m_CurrentEvent = EChaosEvent::DebugSampleFirstEvent;
	void HandleKillAura();
	void HandleReviveAura();
	void HandleLoadRandomMap();
	void HandleRemoveAllWeapons();
	void HandleInfiniteAmmo();
	void HandleMakeAllNPCsInvisible();

public:
	void ExecuteEvent(EChaosEvent event);
	EChaosEvent GetRandomEvent();
	void ExecuteRandomEvent();

	ChaosEvents() {
		eventHandlers = {
			{ EChaosEvent::KillAura, {[this]() { HandleKillAura(); }, 1000, false} },
			{ EChaosEvent::ReviveAura, {[this]() { HandleReviveAura(); }, 1000, false} },
			{ EChaosEvent::LoadRandomMap, {[this]() { HandleLoadRandomMap(); }, 1000, false} },
			{ EChaosEvent::RemoveAllWeapons, {[this]() { HandleRemoveAllWeapons(); }, 1000, false} },
			{ EChaosEvent::InfiniteAmmo, {[this]() { HandleInfiniteAmmo(); }, 1000, false} },
			{ EChaosEvent::MakeAllNPCsInvisible, {[this]() { HandleMakeAllNPCsInvisible(); }, 1000, false} }
		};
	}

	ZSceneData* m_LastLoadedScene = nullptr;
	ZEntitySceneContext* m_LastLoadedSceneContext = nullptr;
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