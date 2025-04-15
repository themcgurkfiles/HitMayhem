#include "ChaosEvents.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZActor.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZInputActionManager.h>
#include "Glacier/ZMath.h"
#include "Glacier/ZCameraEntity.h"
#include <Glacier/ZSetpieceEntity.h>
#include <Glacier/ZItem.h>
#include <Glacier/ZHitman5.h>

#include <Glacier/ZSpatialEntity.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/ZCollision.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZHttp.h>
#include <Glacier/ZPhysics.h>
#include <Glacier/ZContentKitManager.h>
#include <Glacier/ZAction.h>

#include <random>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

void ChaosEvents::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    // This function is called every frame while the game is in play mode.

    for (auto it = activeEffects.begin(); it != activeEffects.end(); ) {
        auto& effect = it->second;
        if (effect.isEffectActive) {
            effect.effectFunction();
            effect.effectDuration--;
            if (effect.effectDuration <= 0) {
                effect.isEffectActive = false;
                Logger::Debug("Effect {} has ended.", static_cast<int>(it->first));
                it = activeEffects.erase(it);  // erase returns a valid next iterator
                continue;
            }
        }
        ++it;
    }
}


void ChaosEvents::ExecuteEvent(EChaosEvent event)
{
	auto it = eventHandlers.find(event);
	if (it != eventHandlers.end()) {
        //it->second.effectFunction();
		m_CurrentEvent = event;
        it->second.isEffectActive = true;
		activeEffects.insert({ event, it->second });
	}
}

ChaosEvents::EChaosEvent ChaosEvents::GetRandomEvent()
{
    std::random_device rd;                       
    std::mt19937 gen(rd());                      
    std::uniform_int_distribution<> dist(
        static_cast<int>(EChaosEvent::DebugSampleFirstEvent) + 1,
        static_cast<int>(EChaosEvent::DebugSampleLastEvent) - 1
    );

    // Get a random event
    EChaosEvent randomEvent = static_cast<EChaosEvent>(dist(gen));
	return randomEvent;
}

void ChaosEvents::ExecuteRandomEvent()
{
	EChaosEvent randomEvent = GetRandomEvent();
    Logger::Debug("Getting Random Event {}...", static_cast<int>(randomEvent));
    ExecuteEvent(randomEvent);
}

void ChaosEvents::HandleKillAura()
{
    //Logger::Debug("HandleKillAura");
    const auto playerPos = SDK()->GetLocalPlayer().m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix().Trans;

    for (int i = 0; i < *Globals::NextActorId; i++)
    {
        auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
        auto actorPos = Globals::ActorManager->m_aActiveActors[i].m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix().Trans;

        auto& actorName = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef->m_sActorName;
        float4 dist = float4::Distance(playerPos, actorPos);

        if (actor && actor->IsAlive() && dist.Length() < 10.0) {

            TEntityRef<IItem> s_Item;
            TEntityRef<ZSetpieceEntity> s_SetPieceEntity;
            Functions::ZActor_KillActor->Call(actor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_Electric, EDeathBehavior::eDB_IMPACT_ANIM);
            //Logger::Debug("Distance to actor {}: {}", actorName, std::to_string(dist.Length()));
            //Logger::Debug("Killing actor: {}", std::to_string(i));
        }
    }
}

void ChaosEvents::HandleReviveAura()
{
    //Logger::Debug("HandleReviveAura");
    const auto playerPos = SDK()->GetLocalPlayer().m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix().Trans;

    for (int i = 0; i < *Globals::NextActorId; i++)
    {
        auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
        auto actorPos = Globals::ActorManager->m_aActiveActors[i].m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix().Trans;

        auto& actorName = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef->m_sActorName;
        float4 dist = float4::Distance(playerPos, actorPos);

        if (actor && dist.Length() < 10.0 && actor->IsDead() || actor->IsPacified()) {

            Functions::ZActor_ReviveActor->Call(actor);
            //Logger::Debug("Distance to actor {}: {}", actorName, std::to_string(dist.Length()));
            //Logger::Debug("Reviving actor: {}", std::to_string(i));
        }
    }
}

void ChaosEvents::HandleLoadRandomMap()
{
    // Work in progress
    Logger::Debug("HandleLoadRandomMap");
	//ZSceneData s_SceneData;
	//s_SceneData.m_sceneName = "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";
    //
	//ZEntitySceneContext* s_SceneContext = Globals::Hitman5Module->m_pEntitySceneContext;
	//s_SceneContext->LoadScene(s_SceneData);
    //Logger::Debug("HandleLoadRandomMap: Loading scene: {}", &m_LastLoadedScene->m_sceneName);
	//m_LastLoadedSceneContext->LoadScene(*m_LastLoadedScene);
}

void ChaosEvents::HandleRemoveAllWeapons()
{
    Logger::Debug("HandleRemoveAllWeapons");
}

void ChaosEvents::HandleInfiniteAmmo()
{
    Logger::Debug("HandleInfiniteAmmo");
}

void ChaosEvents::HandleMakeAllNPCsInvisible()
{
    Logger::Debug("HandleMakeAllNPCsInvisible");
}