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

	const ZMemberDelegate<ChaosEvents, void(const SGameUpdateEvent&)> s_Delegate(m_ChaosEvents, &ChaosEvents::OnFrameUpdate);
    Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);

    // Install a hook to print the name of the scene every time the game loads a new one.
    Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &ModdingTest::OnLoadScene);
}

void ModdingTest::OnDrawMenu() {
    // Toggle our message when the user presses our button.

	if (ImGui::Button(ICON_MD_LOCK_RESET "Trigger a Random Chaos Event")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteRandomEvent(); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Kill Aura")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::KillAura); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Revive Aura")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::ReviveAura); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Invincible")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent:: Make47Invincible); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Infinite Ammo")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::InfiniteAmmo); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Extinguishers")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::SpawnFireExtinguishers); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Rand Teleport")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::Teleport47ToRandChar); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Launch")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::LaunchAllChars); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Test1")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::DebugSampleFirstEvent); }
    }

    if (ImGui::Button(ICON_MD_LOCK_RESET "Test2")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteEvent(ChaosEvents::EChaosEvent::DebugSampleLastEvent); }
    }
}

void ModdingTest::OnDrawUI(bool p_HasFocus) {

}

ModdingTest::~ModdingTest()
{
    const ZMemberDelegate<ChaosEvents, void(const SGameUpdateEvent&)> s_Delegate(m_ChaosEvents, &ChaosEvents::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

DEFINE_PLUGIN_DETOUR(ModdingTest, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData) {
    Logger::Debug("Loading scene: {}", p_SceneData.m_sceneName);
    m_ChaosEvents->m_LastLoadedScene = &p_SceneData;
	m_ChaosEvents->m_LastLoadedSceneContext = th;
    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(ModdingTest);