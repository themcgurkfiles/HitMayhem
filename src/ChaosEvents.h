#pragma once

#include <IPluginInterface.h>

#include <Glacier/SGameUpdateEvent.h>

class ChaosEvents : public IPluginInterface {

public:
	enum class EChaosEvent {
		DebugSampleFirstEvent,
		KillAllNPCs,
		ReviveAllNPCs,
		LoadRandomMap,
		DebugSampleLastEvent
	};

private:
	void ExecuteEvent(EChaosEvent event);
	EChaosEvent GetRandomEvent();
	
	std::unordered_map<EChaosEvent, std::function<void()>> eventHandlers;
	void HandleKillAllNPCs();
	void HandleReviveAllNPCs();
	void HandleLoadRandomMap();

public:
	void ExecuteRandomEvent();

	ChaosEvents() {
		eventHandlers = {
			{ EChaosEvent::KillAllNPCs, [this]() { HandleKillAllNPCs(); } },
			{ EChaosEvent::ReviveAllNPCs, [this]() { HandleReviveAllNPCs(); } },
			{ EChaosEvent::LoadRandomMap, [this]() { HandleLoadRandomMap(); } }
		};
	}

};

/*

Chaos ideas:
- Base time per is 30 sec
- - Optional: Ability to change time between, maybe no chaos for a time



*/