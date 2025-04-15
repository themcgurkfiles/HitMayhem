#pragma once

#include <IPluginInterface.h>
#include "ChaosEvents.h"
#include <Glacier/SGameUpdateEvent.h>

class ModdingTest : public IPluginInterface {
public:
    void OnEngineInitialized() override;
    void OnDrawMenu() override;
    void OnDrawUI(bool p_HasFocus) override;
	~ModdingTest() override;

private:
    DECLARE_PLUGIN_DETOUR(ModdingTest, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& p_SceneData);

private:

    // Chaos Vars
	ChaosEvents* m_ChaosEvents = nullptr;
};

DEFINE_ZHM_PLUGIN(ModdingTest)
