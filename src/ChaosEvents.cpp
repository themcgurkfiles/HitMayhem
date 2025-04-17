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
#include <Glacier/ZInventory.h>

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
            if (effect.effectDuration <= 0) {
                effect.isEffectActive = false;
                Logger::Debug("Effect {} has ended.", static_cast<int>(it->first));
                it = activeEffects.erase(it);  // erase returns a valid next iterator
                continue;
            }

            else
            {
                effect.effectFunction();
                effect.effectDuration--;
                if (effect.justStarted)
                    effect.justStarted = false;
            }
        }
        ++it;
    }
}


void ChaosEvents::ExecuteEvent(EChaosEvent event)
{
	auto it = eventHandlers.find(event);
	if (it != eventHandlers.end()) {
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

void ChaosEvents::CreateCrippleBox()
{
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return;
    }

    constexpr auto s_CrippleBoxFactoryId = ResId<"[modules:/zhm5cripplebox.class].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_CrippleBoxFactory;
    Globals::ResourceManager->GetResourcePtr(s_CrippleBoxFactory, s_CrippleBoxFactoryId, 0);

    if (!s_CrippleBoxFactory) {
        Logger::Debug("Resource is not loaded.");
        return;
    }

    ZEntityRef s_NewCrippleBox;

    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_NewCrippleBox, "", s_CrippleBoxFactory, s_Scene.m_ref, nullptr, -1
    );

    if (!s_NewCrippleBox) {
        Logger::Debug("Failed to spawn entity.");
        return;
    }

    hm5CrippleBox = s_NewCrippleBox.QueryInterface<ZHM5CrippleBox>();
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

void ChaosEvents::HandleInfiniteAmmo()
{
    //Logger::Debug("HandleInfiniteAmmo");

    auto it = activeEffects.find(EChaosEvent::InfiniteAmmo);
    if (it != activeEffects.end()) {
        if (it->second.justStarted && !hm5CrippleBox)
        {
            CreateCrippleBox();
        }
        if (it->second.justStarted && hm5CrippleBox)
        {
            auto s_LocalHitman = SDK()->GetLocalPlayer();

            if (!s_LocalHitman) {
                Logger::Debug("Local player is not alive.");
                return;
            }

            hm5CrippleBox->m_bActivateOnStart = true;
            hm5CrippleBox->m_rHitmanCharacter = s_LocalHitman;
            hm5CrippleBox->m_bLimitedAmmo = false;
            hm5CrippleBox->Activate(0);
        }
        else if (it->second.effectDuration <= 1 && hm5CrippleBox)
        {
            hm5CrippleBox->m_bLimitedAmmo = true;
            hm5CrippleBox->Deactivate(0);
        }
    }
}

void ChaosEvents::HandleMake47Invincible()
{
    //Logger::Debug("HandleMake47Invincible");

    auto player = SDK()->GetLocalPlayer();

    if (player.m_pInterfaceRef->m_bIsInvincible == false)
    {
        player.m_ref.SetProperty("m_bIsInvincible", true);
    }

    auto it = activeEffects.find(EChaosEvent::Make47Invincible);
    if (it != activeEffects.end()) {
        if (it->second.effectDuration <= 1)
        {
            player.m_ref.SetProperty("m_bIsInvincible", false);
            return;
        }
    }

}

void ChaosEvents::HandleRemoveAllWeapons()
{
    Logger::Debug("HandleRemoveAllWeapons");

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        Logger::Debug("No local hitman");
        return;
    }

    if (true) {
        const TArray<TEntityRef<ZCharacterSubcontroller>>* s_Controllers = &s_LocalHitman.m_pInterfaceRef->m_pCharacter.
            m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        auto* s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>(s_Controllers->operator[](6).
            m_pInterfaceRef);

        TArray<ZRepositoryID> s_ModifierIds;
        //s_Inventory->

        return;
    }
}

void ChaosEvents::HandleMakeAllNPCsInvisible()
{
    Logger::Debug("HandleMakeAllNPCsInvisible");
}

void ChaosEvents::HandleMakeAllNPCsEnforcers()
{
	Logger::Debug("HandleMakeAllNPCsEnforcers");
}