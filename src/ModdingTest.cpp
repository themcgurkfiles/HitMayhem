#include "ModdingTest.h"

#include <Logging.h>
#include <IconsMaterialDesign.h>
#include <Globals.h>

#include <Glacier/ZActor.h>
#include <Glacier/ZHM5InputManager.h>
#include <Glacier/ZInputActionManager.h>
#include "Glacier/ZMath.h"
#include "Glacier/ZCameraEntity.h"


#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

void ModdingTest::OnEngineInitialized() {
    Logger::Info("ModdingTest has been initialized!");

    // Register a function to be called on every game frame while the game is in play mode.
    const ZMemberDelegate<ModdingTest, void(const SGameUpdateEvent&)> s_Delegate(this, &ModdingTest::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    // Install a hook to print the name of the scene every time the game loads a new one.
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &ModdingTest::OnLoadScene);
}

ModdingTest::~ModdingTest() {
    // Unregister our frame update function when the mod unloads.
    const ZMemberDelegate<ModdingTest, void(const SGameUpdateEvent&)> s_Delegate(this, &ModdingTest::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void ModdingTest::OnDrawMenu() {
    // Toggle our message when the user presses our button.
    if (ImGui::Button(ICON_MD_LOCAL_FIRE_DEPARTMENT " Kill all NPCs")) {
        if (m_KillAllNPCs == true)
            Logger::Debug("Kills still in progress!");
        else
            m_KillAllNPCs = true;
    }

    if (ImGui::Button(ICON_MD_LOCAL_HOSPITAL " Revive all NPCs")) {
        if (m_ReviveAllNPCs == true)
            Logger::Debug("Revives still in progress!");
        else
            m_ReviveAllNPCs = true;
    }
}

void ModdingTest::OnDrawUI(bool p_HasFocus) {
    if (m_ShowMessage) {
        // Show a window for our mod.
        if (ImGui::Begin("ModdingTest", &m_ShowMessage)) {
            // Only show these when the window is expanded.
            ImGui::Text("Hello from ModdingTest!");
        }
        ImGui::End();
    }
}

void ModdingTest::OnFrameUpdate(const SGameUpdateEvent &p_UpdateEvent) {
    // This function is called every frame while the game is in play mode.
    
    if (m_KillAllNPCs)
    {
        for (int i = 0; i < *Globals::NextActorId; i++)
        {
            auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
            if (actor && actor->IsAlive()) {
                TEntityRef<IItem> s_Item;
                TEntityRef<ZSetpieceEntity> s_SetPieceEntity;
                Functions::ZActor_KillActor->Call(actor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_Burn, EDeathBehavior::eDB_IMPACT_ANIM);
                Logger::Debug("Killing actor: {}", std::to_string(i));
            }
        }
        m_KillAllNPCs = false;
    }

    if (m_ReviveAllNPCs)
    {
		if (countDown >= timeBetweenReviveWaves)
		{
            bool m_HasRevived = false;
            for (int i = 0; i < sizeof(Globals::ActorManager->m_aActiveActors->m_pInterfaceRef); i++)
            {
                auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
                if (actor && actor->IsDead()) {
                    Functions::ZActor_ReviveActor->Call(actor);
                    Logger::Debug("Reviving actor: {}", std::to_string(i));
                    m_HasRevived = true;
                }

                if (i % 1 == 0 && i > 0 && m_HasRevived)
                {
                    break;
                }
            }

            if (!m_HasRevived)
            {
                m_ReviveAllNPCs = false;
                Logger::Debug("No actors to revive!");
            }
            countDown = 0;
		}

        else
        {
            countDown++;
        }
    }
}

DEFINE_PLUGIN_DETOUR(ModdingTest, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData) {
    Logger::Debug("Loading scene: {}", p_SceneData.m_sceneName);
    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(ModdingTest);
