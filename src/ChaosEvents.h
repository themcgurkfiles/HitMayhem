#pragma once

#include <IPluginInterface.h>

#include <Glacier/ZHM5CrippleBox.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include <Glacier/ZCollision.h>

class ChaosEvents : public IPluginInterface {

public:

	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

	// Testing the raycasts
	struct SVector3Less
	{
		bool hasBeenDone = false;
		bool operator()(const SVector3& a, const SVector3& b) const
		{
			if (a.x != b.x) return a.x < b.x;
			if (a.y != b.y) return a.y < b.y;
			return a.z < b.z;
		}
	};
	std::map<SVector3, SVector3, SVector3Less> linesToRender;

	~ChaosEvents();
	
	// The debug events may or may not be needed, at least random won't pick them.
	enum class EChaosEvent { 
		DebugSampleFirstEvent, // Do not assign
		//
		KillAura,
		ReviveAura,
		InfiniteAmmo,
		Make47Invincible,
		Launch47,
		SpawnRandomItem,
		SpawnFireExtinguishers,
		EnableSpaceToJump,
		Teleport47ToRandChar,
		LookingGood47,
		WalkOnAir,
		Give47Boosters,
		//
		DebugSampleLastEvent,   // Do not assign

		// Unused effects for now (work in progress)
		NPCsFriendlyFire, // I have no idea how this would work, but darn would it be funny
		LaunchAllChars, // TODO: FIX THIS EFFECT, IT WORKS BUT THE NPCS DON'T EVER GET UP!
		RemoveAllWeapons,
		MakeAllNPCsInvisible,
		MakeAllNPCsEnforcers,
		TeleportTargetsToRandomChar,
		SpawnRubberDucks,
		SpawnRandomExplosions,

		// Concepts (no funcs made)
		Hivemind, // Concept is to move all enemies when you move: gonna be a pain to do but funny if possible
	};

	struct ChaosEventData {
		std::function<void()> effectFunction;
		std::string effectName = "DEFAULTNAME PLEASE CHANGE";
		// The duration of the effect: setting equal to 1 should only run the effect one time. Zero is treated like the effect has expired.
		int effectDuration = 1000;
		// Call to check if effect is active and running
		bool isEffectActive = false;
		// Bool for effects that need something to be ran once
		bool justStarted = true;
	};

	std::unordered_map<EChaosEvent, ChaosEventData> eventHandlers;
	std::unordered_map<EChaosEvent, ChaosEventData> activeEffects;
	EChaosEvent m_CurrentEvent = EChaosEvent::DebugSampleFirstEvent;

private:
	void HandleKillAura(EChaosEvent eventRef);
	void HandleReviveAura(EChaosEvent eventRef);
	void HandleInfiniteAmmo(EChaosEvent eventRef);
	void HandleMake47Invincible(EChaosEvent eventRef);
	void HandleSpawnFireExtinguishers(EChaosEvent eventRef);
	void HandleRemoveAllWeapons(EChaosEvent eventRef);
	void HandleMakeAllNPCsInvisible(EChaosEvent eventRef);
	void HandleMakeAllNPCsEnforcers(EChaosEvent eventRef);
	void HandleTeleport47ToRandChar(EChaosEvent eventRef);
	void HandleLaunchAllChars(EChaosEvent eventRef);
	void HandleLaunch47(EChaosEvent eventRef);
	void HandleLookingGood47(EChaosEvent eventRef);
	void HandleSpawnRandomItem(EChaosEvent eventRef);
	void HandleTeleportTargetsToRandomChar(EChaosEvent eventRef);
	void HandleEnableSpaceToJump(EChaosEvent eventRef);
	void HandleWalkOnAir(EChaosEvent eventRef);
	void HandleGive47Boosters(EChaosEvent eventRef);
	void HandleNPCsFriendlyFire(EChaosEvent eventRef);

public:
	//--- Event Processing ---//
	void ExecuteEvent(EChaosEvent event);
	EChaosEvent GetRandomEvent();
	void ExecuteRandomEvent();
	bool EventJustStarted(EChaosEvent event);
	bool EventIsEnding(EChaosEvent event);
	bool EventTimeElapsedIsEqualTo(EChaosEvent event, int time);
	bool EventTimeElapsedIsLessThan(EChaosEvent event, int time);
	bool EventTimeElapsedIsGreaterThan(EChaosEvent event, int time);
	bool EventTimeElapsedIsInRange(EChaosEvent event, int lbound, int hbound);
	bool isProcessingEffects = false; // Set false to disable mod running
	bool isProcessingRandEffects = false; // Set false to disable random effects
	//-----------------------//

	//--- Jumping Stuff ---//
	ZInputAction m_JumpAction;
	void ActivateJump();
	void DeactivateJump();
	bool canJump = false;
	bool isJumping = false;
	float jumpCounter = 0.0f;
	float counter = 0.0f;
	float counterLimit = 20;
	//---------------------//

	//--- AirWalk Stuff ---//
	ZInputAction m_AirwalkAction;
	void HandleWalkOnAir();
	bool isAirWalking = false;
	bool canAirWalk = false;
	float maintainedZCoord = 0;
	//---------------------//

	//--- Stuff that could be moved to a helper file at some point: ---//
	void CreateCrippleBox();
	void LoadRepositoryProps(); // Components for Prop Loading
	std::string ConvertDynamicObjectValueTString(const ZDynamicObject& p_DynamicObject);
	std::pair<const std::string, ZRepositoryID> GetRepositoryPropFromIndex(int s_Index);
	std::pair<const std::string, ZRepositoryID> GetRepositoryPropFromName(std::string itemName);
	void InitiateSpawnItem(std::pair<const std::string, ZRepositoryID> s_PropPair, ZSpatialEntity* s_SpatialEntity);
	void InitiateSpawnItem(std::pair<const std::string, ZRepositoryID> s_PropPair, SMatrix& s_PositionMatrix);
	bool m_Running = false;
	bool m_ShowMessage = false;
	bool m_SpawnInWorld = true;
	bool m_IncludeItemsWithoutTitle = false; // Changing this to true makes random items weirder. Could be fun, idk tho, lot of useless stuff spawns in.
	float m_HitmanItemPosition[3] = { 0, 1, 0 };
	TResourcePtr<ZTemplateEntityFactory> m_RepositoryResource;
	std::multimap<std::string, ZRepositoryID> m_RepositoryProps;

	ZSceneData* m_LastLoadedScene = nullptr;
	ZEntitySceneContext* m_LastLoadedSceneContext = nullptr;

	ZHM5CrippleBox* hm5CrippleBox = nullptr;
	//------------------------------------------------------------------//

	ChaosEvents() : m_JumpAction("Jump"), m_AirwalkAction("Airwalk") {
		eventHandlers = {
			// Working effects, ordered from when I got them to work
			{ EChaosEvent::KillAura,               {[this]() { HandleKillAura(EChaosEvent::KillAura); }, "Kill Aura", 3000}},
			{ EChaosEvent::ReviveAura,             {[this]() { HandleReviveAura(EChaosEvent::ReviveAura); }, "Revive Aura", 3000} },
			{ EChaosEvent::InfiniteAmmo,           {[this]() { HandleInfiniteAmmo(EChaosEvent::InfiniteAmmo); }, "Infinite Ammo", 3000} },
			{ EChaosEvent::Make47Invincible,       {[this]() { HandleMake47Invincible(EChaosEvent::Make47Invincible); }, "GODMODE!!!", 3000} },
			{ EChaosEvent::Teleport47ToRandChar,   {[this]() { HandleTeleport47ToRandChar(EChaosEvent::Teleport47ToRandChar); }, "Where In the World is Agent 47?", 500}},
			{ EChaosEvent::LaunchAllChars,         {[this]() { HandleLaunchAllChars(EChaosEvent::LaunchAllChars); }, "Launch all NPCs", 100}},
			{ EChaosEvent::Launch47,               {[this]() { HandleLaunch47(EChaosEvent::Launch47); }, "To The Moon, 47!", 500}},
			{ EChaosEvent::LookingGood47,          {[this]() { HandleLookingGood47(EChaosEvent::LookingGood47); }, "Looking Good, 47!", 500}},
			{ EChaosEvent::SpawnRandomItem,		   {[this]() { HandleSpawnRandomItem(EChaosEvent::SpawnRandomItem); }, "Spawn Random Item", 500} },
			{ EChaosEvent::SpawnFireExtinguishers, {[this]() { HandleSpawnFireExtinguishers(EChaosEvent::SpawnFireExtinguishers); }, "Fire Extinguisher Snake", 1000} },
			{ EChaosEvent::EnableSpaceToJump,	   {[this]() { HandleEnableSpaceToJump(EChaosEvent::EnableSpaceToJump); }, "Hit Space to Jump!", 3000} },
			{ EChaosEvent::WalkOnAir,			   {[this]() { HandleWalkOnAir(EChaosEvent::WalkOnAir); }, "Press F to Walk on Air!", 3000} },
			{ EChaosEvent::Give47Boosters,		   {[this]() { HandleGive47Boosters(EChaosEvent::Give47Boosters); }, "A Well-Needed Boost", 500} },
			
			// Work-in-progress effects, ordered from when I started working on them
			{ EChaosEvent::RemoveAllWeapons, {[this]() { HandleRemoveAllWeapons(EChaosEvent::RemoveAllWeapons); }, "Disarmed", 1} },
			{ EChaosEvent::MakeAllNPCsInvisible, {[this]() { HandleMakeAllNPCsInvisible(EChaosEvent::MakeAllNPCsInvisible); }, "Ghost NPCs", 1000} },
			{ EChaosEvent::MakeAllNPCsEnforcers, {[this]() { HandleMakeAllNPCsEnforcers(EChaosEvent::MakeAllNPCsEnforcers); }, "Upstanding Citizens", 1000} },
			{ EChaosEvent::TeleportTargetsToRandomChar, {[this]() { HandleTeleportTargetsToRandomChar(EChaosEvent::TeleportTargetsToRandomChar); }, "Random Target Teleport", 1}},
			{ EChaosEvent::NPCsFriendlyFire,	   {[this]() { HandleNPCsFriendlyFire(EChaosEvent::NPCsFriendlyFire); }, "NPCs Can Friendly Fire", 1000} },
		};
	}
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