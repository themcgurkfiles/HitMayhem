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

	// Initialize the ChaosEvents class
    m_ChaosEvents = new ChaosEvents();
}

ModdingTest::~ModdingTest() {
    // Unregister our frame update function when the mod unloads.
    const ZMemberDelegate<ModdingTest, void(const SGameUpdateEvent&)> s_Delegate(this, &ModdingTest::OnFrameUpdate);
    Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 1, EUpdateMode::eUpdatePlayMode);
}

void ModdingTest::OnDrawMenu() {
    // Toggle our message when the user presses our button.

	if (ImGui::Button(ICON_MD_LOCK_RESET "Trigger a Random Chaos Event")) {
        if (m_ChaosEvents) { m_ChaosEvents->ExecuteRandomEvent(); }
    }
}

void ModdingTest::OnDrawUI(bool p_HasFocus) {

}

void ModdingTest::OnFrameUpdate(const SGameUpdateEvent &p_UpdateEvent) {
    // This function is called every frame while the game is in play mode.
}

DEFINE_PLUGIN_DETOUR(ModdingTest, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData) {
    Logger::Debug("Loading scene: {}", p_SceneData.m_sceneName);
    return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(ModdingTest);
