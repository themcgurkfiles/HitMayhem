#pragma once

#include <IPluginInterface.h>
#include "ChaosEvents.h"
#include <Glacier/SGameUpdateEvent.h>

class ModdingTest : public IPluginInterface {
public:
    void OnEngineInitialized() override;
    ~ModdingTest() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;

private:
    void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);
    DECLARE_PLUGIN_DETOUR(ModdingTest, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData);

private:
    bool m_ShowMessage = false;

    // Scuffed Revive Stuff
    bool m_ReviveAllNPCs = false;
    int timeBetweenReviveWaves = 500;
	int countDown = 0;

    // Chaos Vars
	ChaosEvents* m_ChaosEvents = nullptr;
};

DEFINE_ZHM_PLUGIN(ModdingTest)
