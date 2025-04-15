#pragma once

#include <IPluginInterface.h>

#include <Glacier/SGameUpdateEvent.h>

class ChaosEvents : public IPluginInterface {

public:
	// The debug events may or may not be needed, at least random won't pick them.
	enum class EChaosEvent { 
		DebugSampleFirstEvent, // Do not assign
		KillAllNPCs,
		ReviveAllNPCs,
		LoadRandomMap,
		RemoveAllWeapons,
		InfiniteAmmo,
		MakeAllNPCsInvisible,
		DebugSampleLastEvent   // Do not assign
	};

private:
	std::unordered_map<EChaosEvent, std::function<void()>> eventHandlers;
	void HandleKillAllNPCs();
	void HandleReviveAllNPCs();
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
			{ EChaosEvent::KillAllNPCs, [this]() { HandleKillAllNPCs(); } },
			{ EChaosEvent::ReviveAllNPCs, [this]() { HandleReviveAllNPCs(); } },
			{ EChaosEvent::LoadRandomMap, [this]() { HandleLoadRandomMap(); } },
			{ EChaosEvent::RemoveAllWeapons, [this]() { HandleRemoveAllWeapons(); } },
			{ EChaosEvent::InfiniteAmmo, [this]() { HandleInfiniteAmmo(); } },
			{ EChaosEvent::MakeAllNPCsInvisible, [this]() { HandleMakeAllNPCsInvisible(); } }
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