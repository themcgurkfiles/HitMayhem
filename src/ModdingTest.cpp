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

    // Initialize the ChaosEvents class
    m_ChaosEvents = new ChaosEvents();

    // custom binds
    const char* binds = "Jump=tap(kb,space); Airwalk=tap(kb,f);";
    if (ZInputActionManager::AddBindings(binds)) {
        Logger::Debug("Successfully added bindings.");
    }
    else {
        Logger::Debug("Failed to add bindings.");
    }


	const ZMemberDelegate<ChaosEvents, void(const SGameUpdateEvent&)> s_Delegate(m_ChaosEvents, &ChaosEvents::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    // Install a hook to print the name of the scene every time the game loads a new one.
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &ModdingTest::OnLoadScene);
}

void ModdingTest::OnDrawMenu() {
    // Toggle our message when the user presses our button.

    if (ImGui::Button(ICON_MD_LOCK_RESET "Toggle Chaos Processing")) {
        if (m_ChaosEvents) { m_ChaosEvents->isProcessingEffects = !m_ChaosEvents->isProcessingEffects; m_ChaosEvents->activeEffects.clear(); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Toggle Timed Random Events")) {
        if (m_ChaosEvents) { m_ChaosEvents->isProcessingRandEffects = !m_ChaosEvents->isProcessingRandEffects; }
    }

	if (ImGui::Button(ICON_MD_LOCK_RESET "Trigger a Random Chaos Event")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteRandomEvent(); }
    }

    //if (ImGui::Button(ICON_MD_LOCK_RESET "Kill Aura")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::KillAura); }
    //}
    //
    //if (ImGui::Button(ICON_MD_LOCK_RESET "Revive Aura")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::ReviveAura); }
    //}
    //
    if (ImGui::Button(ICON_MD_LOCK_RESET "Invincible")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::Make47Invincible); }
    }
    //
    //if (ImGui::Button(ICON_MD_LOCK_RESET "Infinite Ammo")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::InfiniteAmmo); }
    //}
    //
    if (ImGui::Button(ICON_MD_LOCK_RESET "Rand Teleport")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::Teleport47ToRandChar); }
    }
    //
    //if (ImGui::Button(ICON_MD_LOCK_RESET "Launch NPC")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::LaunchAllChars); }
    //}
    //
    //if (ImGui::Button(ICON_MD_LOCK_RESET "Launch 47")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::Launch47); }
    //}
    //
    //if (ImGui::Button(ICON_MD_LOCK_RESET "LookingGood")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::LookingGood47); }
    //}
    //
	//if (ImGui::Button(ICON_MD_LOCK_RESET "RandItem")) {
	//	if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::SpawnRandomItem); }
	//}
    //
    //if (ImGui::Button(ICON_MD_LOCK_RESET "FireExtinguish")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::SpawnFireExtinguishers); }
    //}

    if (ImGui::Button(ICON_MD_LOCK_RESET "Enable Jump")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::EnableSpaceToJump); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Walk On Air")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::WalkOnAir); }
    }

    //if (ImGui::Button(ICON_MD_LOCK_RESET "Give 47 Boosters")) {
    //    if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::Give47Boosters); }
    //}

    if (ImGui::Button(ICON_MD_LOCK_RESET "NPCs Friendly Fire")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::NPCsFriendlyFire); }
    }
}

void ModdingTest::OnDrawUI(bool p_HasFocus) {  
    auto s_ImgGuiIO = ImGui::GetIO();
    
    if (SDK())
    {
        ImGui::SetNextWindowPos(ImVec2(0, 500), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(80, 25), ImVec2(1500, 200));
        ImGui::SetNextWindowBgAlpha(1.f);
        ImGui::Begin("ChaosEvents", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("Time Until Next Effect: ");
		if (m_ChaosEvents)
		{
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", std::to_string(m_ChaosEvents->counterLimit - m_ChaosEvents->counter).c_str());
            
            ImGui::Text("Active Effects:");
            for (auto it = m_ChaosEvents->activeEffects.begin(); it != m_ChaosEvents->activeEffects.end(); ++it) {
				const auto& effect = it->second;
				ImGui::Text("%s", effect.effectName.c_str());
                ImGui::SameLine();

                std::string stringDuration = std::to_string(effect.effectDuration);

				if (stringDuration.length() > 2) {
					stringDuration.insert(stringDuration.length() - 2, ".");
				}
				else {
					stringDuration.insert(0, "0.");
				}

                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", stringDuration.c_str());
			}
		}
    }
    ImGui::End();
}

ModdingTest::~ModdingTest()
{
    if (m_ChaosEvents)
    {
        const ZMemberDelegate<ChaosEvents, void(const SGameUpdateEvent&)> s_Delegate(m_ChaosEvents, &ChaosEvents::OnFrameUpdate);
        Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

        delete m_ChaosEvents;
    }
}

void ModdingTest::OnDraw3D(IRenderer* p_Renderer)
{
	if (m_ChaosEvents) {
        for (auto it = m_ChaosEvents->linesToRender.begin(); it != m_ChaosEvents->linesToRender.end(); ++it)
        {
            p_Renderer->DrawLine3D(
                it->first,
                it->second,
                SVector4(1, 0, 0, 1),
                SVector4(0, 1, 0, 1)
            );
        }
	}
}

DEFINE_PLUGIN_DETOUR(ModdingTest, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData) {
    Logger::Debug("Loading scene: {}", p_SceneData.m_sceneName);
    m_ChaosEvents->m_LastLoadedScene = &p_SceneData;
	m_ChaosEvents->m_LastLoadedSceneContext = th;
    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(ModdingTest);