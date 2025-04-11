#pragma once

#include <IPluginInterface.h>

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

    // Custom mod bools
    bool m_CameraOverride = false;
    bool m_ReviveAllNPCs = false;
	bool m_KillAllNPCs = false;

    int timeBetweenReviveWaves = 100;
	int countDown = 0;
};

DEFINE_ZHM_PLUGIN(ModdingTest)
