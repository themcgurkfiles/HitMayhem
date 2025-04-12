#include "ChaosEvents.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZActor.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZInputActionManager.h>
#include "Glacier/ZMath.h"
#include "Glacier/ZCameraEntity.h"

#include <random>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

void ChaosEvents::ExecuteEvent(EChaosEvent event)
{
	auto it = eventHandlers.find(event);
	if (it != eventHandlers.end()) {
		it->second();
	}
}

ChaosEvents::EChaosEvent ChaosEvents::GetRandomEvent()
{
    std::random_device rd;                       
    std::mt19937 gen(rd());                      
    std::uniform_int_distribution<> dist(
        static_cast<int>(EChaosEvent::KillAllNPCs),
        static_cast<int>(EChaosEvent::LoadRandomMap)
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


void ChaosEvents::HandleKillAllNPCs()
{
    Logger::Debug("HandleKillAllNPCs");
    //for (int i = 0; i < *Globals::NextActorId; i++)
    //{
    //    auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
    //    if (actor && actor->IsAlive()) {
    //        TEntityRef<IItem> s_Item;
    //        TEntityRef<ZSetpieceEntity> s_SetPieceEntity;
    //        Functions::ZActor_KillActor->Call(actor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_Burn, EDeathBehavior::eDB_IMPACT_ANIM);
    //        //Logger::Debug("Killing actor: {}", std::to_string(i));
    //    }
    //}
}

void ChaosEvents::HandleReviveAllNPCs()
{
    Logger::Debug("HandleReviveAllNPCs");

}

void ChaosEvents::HandleLoadRandomMap()
{
    Logger::Debug("HandleLoadRandomMap");

}