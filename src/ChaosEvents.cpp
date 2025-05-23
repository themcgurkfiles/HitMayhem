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
#include <Glacier/ZOutfit.h>

#include <random>

#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZScene.h>

void ChaosEvents::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent) {
    // This function is called every frame while the game is in play mode.
    auto localPlayer = SDK()->GetLocalPlayer();
    if (!localPlayer) {
        if (!activeEffects.empty())
        {
            ResetChaosData();
            Logger::Debug("Couldn't find player, clearing effects...");
        }
        return;
    }

    linesToRender.clear();

    if (!isProcessingEffects) return;

    if (isProcessingRandEffects)
    {
        counter += p_UpdateEvent.m_GameTimeDelta.ToSeconds();
        if (counter >= counterLimit) {
            ExecuteRandomEvent();
            counter = 0;
        }
    }

    for (auto it = activeEffects.begin(); it != activeEffects.end(); ) {
        auto& effect = it->second;

        if (effect.isEffectActive) {
            if (effect.effectDuration <= 0) {
                effect.isEffectActive = false;
                Logger::Debug("Effect '{}' has ended.", it->second.effectName);
                it = activeEffects.erase(it);  // erase returns a valid next iterator
                continue;
            } else if (effect.effectFunction) {
                effect.effectFunction();
                effect.effectDuration -= p_UpdateEvent.m_GameTimeDelta.ToSeconds();
                if (effect.justStarted)
                {
                    Logger::Debug("Effect '{}' has started.", it->second.effectName);
                    effect.justStarted = false;
                }
            }
        }
        ++it;
    }

    if (Functions::ZInputAction_Digital->Call(&m_JumpAction, -1)) {
        //Logger::Debug("Jump input detected.");
		if (canJump && !isJumping) {
            ActivateJump();
		}
    }

    if (Functions::ZInputAction_Digital->Call(&m_AirwalkAction, -1)) {
        //Logger::Debug("Airwalk input detected.");
        if (!canAirWalk) return;
        isAirWalking = !isAirWalking;

        auto s_LocalHitman = SDK()->GetLocalPlayer();
        if (!s_LocalHitman) {
            Logger::Error("No local hitman.");
            return;
        }

        auto s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
        SMatrix s_HitmanWorldMatrix = s_SpatialEntity->GetWorldMatrix();
        maintainedZCoord = s_HitmanWorldMatrix.Trans.z + 1;
    }

    if (isAirWalking)
    {
        HandleWalkOnAir();
    }

    if (isJumping)
    {
        jumpCounter += p_UpdateEvent.m_GameTimeDelta.ToSeconds();
        if (jumpCounter >= 2.3) {
            DeactivateJump();
            isJumping = false;
            jumpCounter = 0;
        }
    }
}

void ChaosEvents::ResetChaosData()
{
    Logger::Debug("ChaosEvents destructor called.");

    counter = 0;
    activeEffects.clear();
    linesToRender.clear();
    m_Running = false;
    canJump = false;
    isJumping = false;
    isAirWalking = false;
    canAirWalk = false;

    if (hm5CrippleBox)
    {
        hm5CrippleBox->m_bActivateOnStart = false;
        hm5CrippleBox = nullptr;
    }
}

ChaosEvents::~ChaosEvents()
{
    ResetChaosData();

    // Only clear these values on destruct
    eventHandlers.clear();
    m_RepositoryProps.clear();
    hm5CrippleBox->m_bActivateOnStart = false;
    hm5CrippleBox = nullptr;
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

void ChaosEvents::LoadRepositoryProps()
{
    Logger::Info("Loading repository (your game will freeze shortly)");

    std::string s_IncludedCategories[] = {
        "assaultrifle", "sniperrifle", "melee", "explosives", "tool", "pistol", "shotgun", "suitcase", "smg", "distraction", "poison", "container",
        "INVALID_CATEGORY_ICON" // <- debatable, makes it more random but also kind of wonky
    };

    if (m_RepositoryResource.m_nResourceIndex == -1)
    {
        const auto s_ID = ResId<"[assembly:/repository/pro.repo].pc_repo">;

        Globals::ResourceManager->GetResourcePtr(m_RepositoryResource, s_ID, 0);
    }

    if (m_RepositoryResource.GetResourceInfo().status == RESOURCE_STATUS_VALID)
    {
        const auto s_RepositoryData = static_cast<THashMap<ZRepositoryID, ZDynamicObject, TDefaultHashMapPolicy<ZRepositoryID>>*>(m_RepositoryResource.GetResourceData());

        for (auto it = s_RepositoryData->begin(); it != s_RepositoryData->end(); ++it)
        {
            const ZDynamicObject* s_DynamicObject = &it->second;
            const TArray<SDynamicObjectKeyValuePair>* s_Entries = s_DynamicObject->As<TArray<SDynamicObjectKeyValuePair>>();

            std::string s_Id;

            bool s_HasTitle = false;
            bool s_Included = true;
            std::string s_TitleToAdd;
            ZRepositoryID s_RepoIdToAdd("");

            for (size_t i = 0; i < s_Entries->size(); ++i)
            {
                std::string s_Key = s_Entries->operator[](i).sKey.c_str();

                if (s_Key == "ID_")
                {
                    s_Id = ConvertDynamicObjectValueTString(s_Entries->at(i).value);
                }

                if (s_Key == "Title")
                {
                    s_HasTitle = true;
                    std::string s_Title = ConvertDynamicObjectValueTString(s_Entries->at(i).value);

                    s_TitleToAdd = s_Title;
                    s_RepoIdToAdd = ZRepositoryID(s_Id.c_str());

                    if (s_Title.size() < 1 && !m_IncludeItemsWithoutTitle) s_Included = false;
                }

                if (s_Key == "InventoryCategoryIcon") {
                    std::string s_Category = ConvertDynamicObjectValueTString(s_Entries->at(i).value);
                    bool s_CategoryMatched = false;

                    std::transform(s_Category.begin(), s_Category.end(), s_Category.begin(), ::toupper);

                    for (std::string s_IncludedCategory : s_IncludedCategories) {
                        std::transform(s_IncludedCategory.begin(), s_IncludedCategory.end(), s_IncludedCategory.begin(), ::toupper);
                        if (s_IncludedCategory == s_Category) s_CategoryMatched = true;
                    }

                    if (!s_CategoryMatched) s_Included = false;
                }

                if (s_Key == "IsHitmanSuit") {
                    s_Included = false;
                    break;
                }
            }

            if (s_Included && (s_HasTitle || m_IncludeItemsWithoutTitle)) {
                m_RepositoryProps.insert(std::make_pair(s_TitleToAdd, s_RepoIdToAdd));
            }
        }
    }
}

std::string ChaosEvents::ConvertDynamicObjectValueTString(const ZDynamicObject& p_DynamicObject)
{
    std::string s_Result;
    const IType* s_Type = p_DynamicObject.m_pTypeID->typeInfo();

    if (strcmp(s_Type->m_pTypeName, "ZString") == 0)
    {
        const auto s_Value = p_DynamicObject.As<ZString>();
        s_Result = s_Value->c_str();
    }
    else if (strcmp(s_Type->m_pTypeName, "bool") == 0)
    {
        if (*p_DynamicObject.As<bool>())
        {
            s_Result = "true";
        }
        else
        {
            s_Result = "false";
        }
    }
    else if (strcmp(s_Type->m_pTypeName, "float64") == 0)
    {
        double value = *p_DynamicObject.As<double>();

        s_Result = std::to_string(value).c_str();
    }
    else
    {
        s_Result = s_Type->m_pTypeName;
    }

    return s_Result;
}

std::pair<const std::string, ZRepositoryID> ChaosEvents::GetRepositoryPropFromIndex(int s_Index)
{
    int s_CurrentIndex = 0;
    for (auto it = m_RepositoryProps.begin(); it != m_RepositoryProps.end(); ++it) {
        if (s_CurrentIndex == s_Index) {
            return *it;
        }
        ++s_CurrentIndex;
    }
    Logger::Error("repo index out of bounds");
}

std::pair<const std::string, ZRepositoryID> ChaosEvents::GetRepositoryPropFromName(std::string itemName)
{
    int s_CurrentIndex = 0;
    for (auto it = m_RepositoryProps.begin(); it != m_RepositoryProps.end(); ++it) {
        if (it->first == itemName) {
            return *it;
        }
        ++s_CurrentIndex;
    }
    Logger::Error("repo index out of bounds");
}

void ChaosEvents::InitiateSpawnItem(std::pair<const std::string, ZRepositoryID> s_PropPair, ZSpatialEntity* s_SpatialEntity)
{
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }
    
    if (m_SpawnInWorld) {
        Logger::Info("Spawning in world: {}", s_PropPair.first);
        

        const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;
        if (!s_Scene) {
            Logger::Warn("no scene loaded");
            return;
        }

        const auto s_ItemSpawnerID = ResId<"[modules:/zitemspawner.class].pc_entitytype">;
        const auto s_ItemRepoKeyID = ResId<"[modules:/zitemrepositorykeyentity.class].pc_entitytype">;

        TResourcePtr<ZTemplateEntityFactory> s_Resource, s_Resource2;

        Globals::ResourceManager->GetResourcePtr(s_Resource, s_ItemSpawnerID, 0);
        Globals::ResourceManager->GetResourcePtr(s_Resource2, s_ItemRepoKeyID, 0);

        if (!s_Resource)
        {
            Logger::Error("resource not loaded");
            return;
        }

        ZEntityRef s_ItemSpawnerEntity, s_ItemRepoKey;

        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_ItemSpawnerEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);
        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_ItemRepoKey, "", s_Resource2, s_Scene.m_ref, nullptr, -1);

        if (!s_ItemSpawnerEntity)
        {
            Logger::Error("failed to spawn item spawner");
            return;
        }

        if (!s_ItemRepoKey)
        {
            Logger::Error("failed to spawn item repo key entity");
            return;
        }

        const auto s_ItemSpawner = s_ItemSpawnerEntity.QueryInterface<ZItemSpawner>();
        s_ItemSpawner->m_ePhysicsMode = ZItemSpawner::EPhysicsMode::EPM_DYNAMIC;
        s_ItemSpawner->m_rMainItemKey.m_ref = s_ItemRepoKey;
        s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef = s_ItemRepoKey.QueryInterface<ZItemRepositoryKeyEntity>();
        s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef->m_RepositoryId = s_PropPair.second;
        s_ItemSpawner->m_bUsePlacementAttach = false;
        s_ItemSpawner->m_eDisposalTypeOverwrite = EDisposalType::DISPOSAL_HIDE;
        s_ItemSpawner->SetWorldMatrix(s_SpatialEntity->GetWorldMatrix());

        Functions::ZItemSpawner_RequestContentLoad->Call(s_ItemSpawner);
    }
    else {
        Logger::Info("Adding to inventory: {} {}", s_PropPair.first, s_PropPair.second.ToString());
        const TArray<TEntityRef<ZCharacterSubcontroller>>* s_Controllers = &s_LocalHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        auto* s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>(s_Controllers->operator[](6).m_pInterfaceRef);

        TArray<ZRepositoryID> s_ModifierIds;
        Functions::ZCharacterSubcontrollerInventory_AddDynamicItemToInventory->Call(s_Inventory, s_PropPair.second, "", &s_ModifierIds, 2);
    }
}

void ChaosEvents::InitiateSpawnItem(std::pair<const std::string, ZRepositoryID> s_PropPair, SMatrix& s_PositionMatrix)
{
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }

    if (m_SpawnInWorld) {
        Logger::Info("Spawning in world: {}", s_PropPair.first);


        const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;
        if (!s_Scene) {
            Logger::Warn("no scene loaded");
            return;
        }

        const auto s_ItemSpawnerID = ResId<"[modules:/zitemspawner.class].pc_entitytype">;
        const auto s_ItemRepoKeyID = ResId<"[modules:/zitemrepositorykeyentity.class].pc_entitytype">;

        TResourcePtr<ZTemplateEntityFactory> s_Resource, s_Resource2;

        Globals::ResourceManager->GetResourcePtr(s_Resource, s_ItemSpawnerID, 0);
        Globals::ResourceManager->GetResourcePtr(s_Resource2, s_ItemRepoKeyID, 0);

        if (!s_Resource)
        {
            Logger::Error("resource not loaded");
            return;
        }

        ZEntityRef s_ItemSpawnerEntity, s_ItemRepoKey;

        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_ItemSpawnerEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1);
        Functions::ZEntityManager_NewEntity->Call(Globals::EntityManager, s_ItemRepoKey, "", s_Resource2, s_Scene.m_ref, nullptr, -1);

        if (!s_ItemSpawnerEntity)
        {
            Logger::Error("failed to spawn item spawner");
            return;
        }

        if (!s_ItemRepoKey)
        {
            Logger::Error("failed to spawn item repo key entity");
            return;
        }

        const auto s_ItemSpawner = s_ItemSpawnerEntity.QueryInterface<ZItemSpawner>();

        s_ItemSpawner->m_ePhysicsMode = ZItemSpawner::EPhysicsMode::EPM_KINEMATIC;
        s_ItemSpawner->m_rMainItemKey.m_ref = s_ItemRepoKey;
        s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef = s_ItemRepoKey.QueryInterface<ZItemRepositoryKeyEntity>();
        s_ItemSpawner->m_rMainItemKey.m_pInterfaceRef->m_RepositoryId = s_PropPair.second;
        s_ItemSpawner->m_bUsePlacementAttach = false;
        s_ItemSpawner->m_eDisposalTypeOverwrite = EDisposalType::DISPOSAL_HIDE;
        s_ItemSpawner->SetWorldMatrix(s_PositionMatrix);

        Functions::ZItemSpawner_RequestContentLoad->Call(s_ItemSpawner);
    }
    else {
        Logger::Info("Adding to inventory: {} {}", s_PropPair.first, s_PropPair.second.ToString());
        const TArray<TEntityRef<ZCharacterSubcontroller>>* s_Controllers = &s_LocalHitman.m_pInterfaceRef->m_pCharacter.m_pInterfaceRef->m_rSubcontrollerContainer.m_pInterfaceRef->m_aReferencedControllers;
        auto* s_Inventory = static_cast<ZCharacterSubcontrollerInventory*>(s_Controllers->operator[](6).m_pInterfaceRef);

        TArray<ZRepositoryID> s_ModifierIds;
        Functions::ZCharacterSubcontrollerInventory_AddDynamicItemToInventory->Call(s_Inventory, s_PropPair.second, "", &s_ModifierIds, 2);
    }
}

ZActor* ChaosEvents::SpawnNPC(const std::string& s_NpcName, const ZRepositoryID& repositoryID, const TEntityRef<ZGlobalOutfitKit>* p_GlobalOutfitKit, uint8_t n_CurrentCharacterSetIndex, const std::string& s_CurrentcharSetCharacterType, uint8_t p_CurrentOutfitVariationIndex)
{
    const auto s_Scene = Globals::Hitman5Module->m_pEntitySceneContext->m_pScene;

    if (!s_Scene) {
        Logger::Debug("Scene not loaded.");
        return nullptr;
    }

    const auto s_RuntimeResourceId = ResId<
        "[assembly:/templates/gameplay/ai2/actors.template?/npcactor.entitytemplate].pc_entitytype">;

    TResourcePtr<ZTemplateEntityFactory> s_Resource;
    Globals::ResourceManager->GetResourcePtr(s_Resource, s_RuntimeResourceId, 0);

    if (!s_Resource) {
        Logger::Debug("Resource is not loaded.");
        return nullptr;
    }

    ZEntityRef s_NewEntity;
    Functions::ZEntityManager_NewEntity->Call(
        Globals::EntityManager, s_NewEntity, "", s_Resource, s_Scene.m_ref, nullptr, -1
    );

    if (!s_NewEntity) {
        Logger::Debug("Could not spawn entity.");
        return nullptr;
    }

    auto s_LocalHitman = SDK()->GetLocalPlayer();

    if (!s_LocalHitman) {
        Logger::Debug("No local hitman.");
        return nullptr;
    }

    ZActor* actor = s_NewEntity.QueryInterface<ZActor>();

    actor->m_sActorName = s_NpcName;
    actor->m_bStartEnabled = true;
    actor->m_nOutfitCharset = n_CurrentCharacterSetIndex;
    actor->m_nOutfitVariation = p_CurrentOutfitVariationIndex;
    actor->m_OutfitRepositoryID = repositoryID;
    actor->m_eRequiredVoiceVariation = EActorVoiceVariation::eAVV_Undefined;

    actor->Activate(0);

    ZSpatialEntity* s_ActorSpatialEntity = s_NewEntity.QueryInterface<ZSpatialEntity>();
    ZSpatialEntity* s_HitmanSpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();

    s_ActorSpatialEntity->SetWorldMatrix(s_HitmanSpatialEntity->GetWorldMatrix());

    if (p_GlobalOutfitKit) {
        Functions::ZActor_SetOutfit->Call(actor, *p_GlobalOutfitKit, n_CurrentCharacterSetIndex, p_CurrentOutfitVariationIndex, false);
    }

    return actor;
}

void ChaosEvents::ActivateJump()
{
	auto s_LocalHitman = SDK()->GetLocalPlayer();
	if (!s_LocalHitman) {
		Logger::Debug("No local hitman.");
		return;
	}

    isJumping = true;
    Functions::ZHM5BaseCharacter_ActivatePoweredRagdoll->Call(
        SDK()->GetLocalPlayer().m_pInterfaceRef, 0.3f, true, false, 0.15f, false);

    auto* localRagdoller = SDK()->GetLocalPlayer().m_pInterfaceRef->m_pRagdollHandler;
    if (localRagdoller)
    {
        Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(
            localRagdoller, float4(0, 0, 0, 0), float4(0, 150, 250, 1), 1, false);
    }
}

void ChaosEvents::DeactivateJump()
{
	auto s_LocalHitman = SDK()->GetLocalPlayer();
	if (!s_LocalHitman) {
		Logger::Debug("No local hitman.");
		return;
	}
	if (isJumping) {
		isJumping = false;
		Functions::ZHM5BaseCharacter_DeactivateRagdoll->Call(SDK()->GetLocalPlayer().m_pInterfaceRef);
        const auto s_Animator = SDK()->GetLocalPlayer().m_pInterfaceRef->m_Animator.QueryInterface<ZHM5Animator>();
        auto s_Time = 1.f;
        Functions::ZHM5Animator_ActivateRagdollToAnimationBlend->Call(s_Animator, &s_Time);
	}
}

void ChaosEvents::HandleWalkOnAir()
{
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }

    if (isAirWalking)
    {
        auto s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
        SMatrix s_HitmanWorldMatrix = s_SpatialEntity->GetWorldMatrix();
        s_HitmanWorldMatrix.Trans.z = maintainedZCoord;
        s_SpatialEntity->SetWorldMatrix(s_HitmanWorldMatrix);
    }
}

bool ChaosEvents::EventJustStarted(EChaosEvent event)
{
	auto it = activeEffects.find(event);
	if (it != activeEffects.end()) {
		return it->second.justStarted;
	}
	return false;
}

bool ChaosEvents::EventIsEnding(EChaosEvent event)
{
    auto it = activeEffects.find(event);
    if (it != activeEffects.end()) {
        if (it->second.effectDuration <= 1)
        {
            return true;
        }
    }
    return false;
}

bool ChaosEvents::EventTimeElapsedIsEqualTo(EChaosEvent event, int time)
{
    auto it = activeEffects.find(event);
    if (it != activeEffects.end()) {
        if (it->second.effectDuration == time)
        {
            return true;
        }
    }
    return false;
}

bool ChaosEvents::EventTimeElapsedIsLessThan(EChaosEvent event, int time)
{
    auto it = activeEffects.find(event);
    if (it != activeEffects.end()) {
        int timeEffectRan = eventHandlers.find(event)->second.effectDuration - it->second.effectDuration;
        if (timeEffectRan < time)
        {
            return true;
        }
    }
    return false;
}

bool ChaosEvents::EventTimeElapsedIsGreaterThan(EChaosEvent event, int time)
{
    auto it = activeEffects.find(event);
    if (it != activeEffects.end()) {
        int timeEffectRan = eventHandlers.find(event)->second.effectDuration - it->second.effectDuration;
        if (timeEffectRan > time)
        {
            return true;
        }
    }
    return false;
}

bool ChaosEvents::EventTimeElapsedIsInRange(EChaosEvent event, int lbound, int hbound)
{
    auto it = activeEffects.find(event);
    if (it != activeEffects.end()) {
        int timeEffectRan = eventHandlers.find(event)->second.effectDuration - it->second.effectDuration;
        if (timeEffectRan <= hbound && timeEffectRan >= lbound)
        {
            return true;
        }
    }
    return false;
}


// Effect Functions can be found below:

void ChaosEvents::HandleKillAura(EChaosEvent eventRef)
{
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

void ChaosEvents::HandleReviveAura(EChaosEvent eventRef)
{
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

void ChaosEvents::HandleInfiniteAmmo(EChaosEvent eventRef)
{   
    if (EventJustStarted(eventRef))
    {
        CreateCrippleBox();
        auto s_LocalHitman = SDK()->GetLocalPlayer();
        hm5CrippleBox->m_bActivateOnStart = true;
        hm5CrippleBox->m_rHitmanCharacter = s_LocalHitman;
        hm5CrippleBox->m_bLimitedAmmo = false;
        hm5CrippleBox->Activate(0);
    }

    if (EventIsEnding(eventRef))
    {
        hm5CrippleBox->Deactivate(0);
        hm5CrippleBox->m_bActivateOnStart = false;
    }
}

void ChaosEvents::HandleMake47Invincible(EChaosEvent eventRef)
{
    auto player = SDK()->GetLocalPlayer();
	if (!player) {
		Logger::Debug("No local hitman");
		return;
	}

    if (EventJustStarted(eventRef) && player.m_pInterfaceRef->m_bIsInvincible == false)
    {
        player.m_ref.SetProperty("m_bIsInvincible", true);
    }

    if (EventIsEnding(eventRef))
    {
        player.m_ref.SetProperty("m_bIsInvincible", false);
    }
}

void ChaosEvents::HandleRemoveAllWeapons(EChaosEvent eventRef)
{
    // WIP: NOT CURRENTLY WORKING
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

void ChaosEvents::HandleMakeAllNPCsInvisible(EChaosEvent eventRef)
{

}

void ChaosEvents::HandleMakeAllNPCsEnforcers(EChaosEvent eventRef)
{

}

void ChaosEvents::HandleTeleport47ToRandChar(EChaosEvent eventRef)
{
    if (!EventJustStarted(eventRef))
        return;
    
    int randomIndex = rand() % *Globals::NextActorId;
    for (int i = 0; i < *Globals::NextActorId; i++)
    {
        auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
        auto actorPos = Globals::ActorManager->m_aActiveActors[i].m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix().Trans;

        if (i == randomIndex) {
            if (auto s_LocalHitman = SDK()->GetLocalPlayer()) {
                ZSpatialEntity* s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
                SMatrix s_WorldMatrix = s_SpatialEntity->GetWorldMatrix();
                s_WorldMatrix.Trans = actorPos;
                s_SpatialEntity->SetWorldMatrix(s_WorldMatrix);
            }
        }
    }
}

void ChaosEvents::HandleLaunchAllChars(EChaosEvent eventRef)
{
    if (EventJustStarted(eventRef))
    {
        for (int i = 0; i < *Globals::NextActorId; i++)
        {
            auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;

            if (!actor)
            {
                Logger::Debug("Actor not checking correctly");
                return;
            }

            ZSpatialEntity* s_SpatialEntity = Globals::ActorManager->m_aActiveActors[i].m_ref.QueryInterface<ZSpatialEntity>();
            SMatrix s_WorldMatrix = s_SpatialEntity->GetWorldMatrix();

            Functions::ZHM5BaseCharacter_ActivatePoweredRagdoll->Call(
                actor, 0.3f, true, false, 0.15f, false
            );

            auto* ragdoller = actor->m_pRagdollHandler;
            if (ragdoller)
            {
                Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(
                    ragdoller, float4(0, 0, 0, 0), float4(0, 0, 800, 1), i, false
                );
            }
        }
    }

    if (EventIsEnding(eventRef))
    {
        for (int i = 0; i < *Globals::NextActorId; i++)
        {

            auto& actor = Globals::ActorManager->m_aActiveActors[i];
            Functions::ZHM5BaseCharacter_DeactivateRagdoll->Call(actor.m_pInterfaceRef);

            auto* ragdoller = actor.m_pInterfaceRef->m_pRagdollHandler;
            if (ragdoller)
            {
                Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(
                    ragdoller, float4(0, 0, 0, 0), float4(0, 0, 50, 1), i, false
                );
            }
        }
    }
}

void ChaosEvents::HandleLaunch47(EChaosEvent eventRef)
{
    if (EventJustStarted(eventRef))
    {
        Functions::ZHM5BaseCharacter_ActivatePoweredRagdoll->Call(
            SDK()->GetLocalPlayer().m_pInterfaceRef, 0.3f, true, false, 0.15f, false);

        auto* localRagdoller = SDK()->GetLocalPlayer().m_pInterfaceRef->m_pRagdollHandler;
        if (localRagdoller)
        {
            Functions::ZRagdollHandler_ApplyImpulseOnRagdoll->Call(
                localRagdoller, float4(0, 0, 0, 0), float4(0, 0, 1600, 1), 1, false);
        }
    }

    else if (EventIsEnding(eventRef))
    {
        Functions::ZHM5BaseCharacter_DeactivateRagdoll->Call(SDK()->GetLocalPlayer().m_pInterfaceRef);
        const auto s_Animator = SDK()->GetLocalPlayer().m_pInterfaceRef->m_Animator.QueryInterface<ZHM5Animator>();
        auto s_Time = 1.f;
        Functions::ZHM5Animator_ActivateRagdollToAnimationBlend->Call(s_Animator, &s_Time);
    }
}

void ChaosEvents::HandleLookingGood47(EChaosEvent eventRef)
{
    if (!EventJustStarted(eventRef))
        return;
    
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
    int i = 0;
	int randomIndex = rand() % s_ContentKitManager->m_repositoryGlobalOutfitKits.size();
    for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); 
        it != s_ContentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
    {
        if (i == randomIndex)
        {
            Logger::Debug("Outfit: {}", (it->second.m_pInterfaceRef->m_sTitle));
            Functions::ZHitman5_SetOutfit->Call(
                s_LocalHitman.m_pInterfaceRef,
                it->second, 0, 1, false, false
            );
            return;
        }

        i++;
    }
}

void ChaosEvents::HandleSpawnRandomItem(EChaosEvent eventRef)
{
    if (!EventJustStarted(eventRef))
        return;
    
    if (m_RepositoryProps.size() == 0)
    {
        LoadRepositoryProps();
    }

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    size_t s_RandomIndex = std::rand() % m_RepositoryProps.size();
    auto s_PropPair = GetRepositoryPropFromIndex(s_RandomIndex);

    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }

    auto s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    SMatrix s_HitmanWorldMatrix = s_SpatialEntity->GetWorldMatrix();
    s_HitmanWorldMatrix.Trans += float4(0, 0, 1, 0);
    m_SpawnInWorld = true;
    InitiateSpawnItem(s_PropPair, s_HitmanWorldMatrix);
}

void ChaosEvents::HandleSpawnFireExtinguishers(EChaosEvent eventRef)
{
    if (EventTimeElapsedIsGreaterThan(eventRef, 200)) return;
    
    if (m_RepositoryProps.size() == 0)
    {
        LoadRepositoryProps();
    }
    
    auto s_PropPair = GetRepositoryPropFromName("Fire Extinguisher");
    if (s_PropPair.second == ZRepositoryID("")) {
        Logger::Error("Fire Extinguisher not found in repository.");
        return;
    }
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }

    auto s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
	SMatrix s_HitmanWorldMatrix = s_SpatialEntity->GetWorldMatrix();
    s_HitmanWorldMatrix.Trans += float4(0, 0, 2, 0);
    m_SpawnInWorld = true;
    InitiateSpawnItem(s_PropPair, s_HitmanWorldMatrix);
}

void ChaosEvents::HandleTeleportTargetsToRandomChar(EChaosEvent eventRef)
{
    if (!EventJustStarted(eventRef))
        return;
    
    for (int i = 0; i < *Globals::NextActorId; i++)
    {
        auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
        ZSpatialEntity* s_SpatialEntity = Globals::ActorManager->m_aActiveActors[i].m_ref.QueryInterface<ZSpatialEntity>();
        SMatrix s_WorldMatrix = s_SpatialEntity->GetWorldMatrix();

        // check if actors are targets:
        actor->GetType();

    }
}

void ChaosEvents::HandleEnableSpaceToJump(EChaosEvent eventRef)
{
    if (EventJustStarted(eventRef))
    {
		canJump = true;
    }
	else if (EventIsEnding(eventRef))
	{
		canJump = false;
	}
}

void ChaosEvents::HandleWalkOnAir(EChaosEvent eventRef)
{
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }

    if (EventJustStarted(eventRef))
    {
        canAirWalk = true;
    }
    else if (EventIsEnding(eventRef))
    {
        canAirWalk = false;
        isAirWalking = false;
        maintainedZCoord = 0;
    }
}

void ChaosEvents::HandleGive47Boosters(EChaosEvent eventRef)
{
    if (EventTimeElapsedIsGreaterThan(eventRef, 2)) return;

    if (m_RepositoryProps.size() == 0)
    {
        LoadRepositoryProps();
    }

    auto s_PropPair = GetRepositoryPropFromName("Octane Booster");
    if (s_PropPair.second == ZRepositoryID("")) {
        Logger::Error("Octane Booster not found in repository.");
        return;
    }
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    if (!s_LocalHitman) {
        Logger::Error("No local hitman.");
        return;
    }

    auto s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    SMatrix s_HitmanWorldMatrix = s_SpatialEntity->GetWorldMatrix();
    m_SpawnInWorld = false;
    InitiateSpawnItem(s_PropPair, s_HitmanWorldMatrix);
}

void ChaosEvents::HandleNPCsFriendlyFire(EChaosEvent eventRef)
{   
    ZRayQueryOutput s_RayOutput{};
    auto actor = SDK()->GetLocalPlayer();

    if (true)
    {
        SMatrix s_WorldMatrix = actor.m_ref.QueryInterface<ZSpatialEntity>()->GetWorldMatrix();
        float4 s_ForwardDirection = float4(
            -s_WorldMatrix.YAxis.x, -s_WorldMatrix.YAxis.y, -s_WorldMatrix.YAxis.z, -s_WorldMatrix.YAxis.w
        );

        float4 s_From = s_WorldMatrix.Trans;
        float4 s_To = s_WorldMatrix.Trans;
        s_From.z += 0.75f;
        s_To = s_From + s_ForwardDirection * 50.0f;

        if (!*Globals::CollisionManager) {
            Logger::Error("Collision manager not found.");
            return;
        }

        ZRayQueryInput s_RayInput{
            .m_vFrom = s_From,
            .m_vTo = s_To,
        };

        // Uncomment for Debug Lines:
        SVector3 s_FromPoint = { s_From.x, s_From.y, s_From.z };
        SVector3 s_ToPoint = { s_To.x, s_To.y, s_To.z };
        linesToRender[s_FromPoint] = s_ToPoint;

        if (!(*Globals::CollisionManager)->RayCastClosestHit(s_RayInput, &s_RayOutput)) {
            Logger::Error("Raycast failed.");
            return;
        }

        if (s_RayOutput.m_BlockingEntity) {
            const auto& s_Interfaces = *s_RayOutput.m_BlockingEntity->GetType()->m_pInterfaces;
        }
        Logger::Debug("Raycast result: {} {}", fmt::ptr(&s_RayOutput), s_RayOutput.m_vPosition);

        auto m_SelectedEntity = s_RayOutput.m_BlockingEntity;

        if (m_SelectedEntity.GetOwningEntity().HasInterface<ZCharacterTemplateAspect>()) {
            if (ZActor* tempSelectedActor = m_SelectedEntity.GetOwningEntity().QueryInterface<ZActor>()) {
                if (tempSelectedActor->IsAlive())
                {
                    m_SelectedEntity = tempSelectedActor->m_rCharacter.m_ref;
                    TEntityRef<IItem> s_Item;
                    TEntityRef<ZSetpieceEntity> s_SetPieceEntity;
                    //Logger::Debug("Killing actor: {}", tempSelectedActor->m_sActorName);
                    Functions::ZActor_KillActor->Call(tempSelectedActor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_Shoot, EDeathBehavior::eDB_IMPACT_ANIM);

                    SVector3 s_FromPoint = { s_From.x, s_From.y, s_From.z };
                    SVector3 s_ToPoint = { s_To.x, s_To.y, s_To.z };
                    linesToRender[s_FromPoint] = s_ToPoint;
                }
            }
        }

        if (m_SelectedEntity.HasInterface<ZActor>()) {
            if (ZActor* s_Actor = m_SelectedEntity.QueryInterface<ZActor>()) {
                if (s_Actor->IsAlive())
                {
                    m_SelectedEntity = s_Actor->m_rCharacter.m_ref;
                    TEntityRef<IItem> s_Item;
                    TEntityRef<ZSetpieceEntity> s_SetPieceEntity;
                    //Logger::Debug("Killing actor: {}", s_Actor->m_sActorName);
                    Functions::ZActor_KillActor->Call(s_Actor, s_Item, s_SetPieceEntity, EDamageEvent::eDE_Shoot, EDeathBehavior::eDB_IMPACT_ANIM);

                    SVector3 s_FromPoint = { s_From.x, s_From.y, s_From.z };
                    SVector3 s_ToPoint = { s_To.x, s_To.y, s_To.z };
                    linesToRender[s_FromPoint] = s_ToPoint;
                }
            }
        }
    }
}

void ChaosEvents::BecomeTheKashmirian(EChaosEvent eventRef)
{
	if (!EventJustStarted(eventRef))
		return;

    if (m_RepositoryProps.size() == 0)
    {
        LoadRepositoryProps();
    }
    
    auto s_LocalHitman = SDK()->GetLocalPlayer();
    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
    for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); it != s_ContentKitManager->m_repositoryGlobalOutfitKits.end(); ++it)
    {
        if (it->second.m_pInterfaceRef->m_sTitle == "Kashmirian")
        {
            Logger::Debug("Outfit: {}", (it->second.m_pInterfaceRef->m_sTitle));
            Functions::ZHitman5_SetOutfit->Call(
                s_LocalHitman.m_pInterfaceRef,
                it->second, 0, 1, false, false
            );
        }
    }

    auto s_PropPair = GetRepositoryPropFromName("Druzhina");

    if (s_PropPair.second == ZRepositoryID("")) {
        Logger::Error("Druzhina 34 not found in repository.");
    }

    else
    {
        auto s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
        SMatrix s_HitmanWorldMatrix = s_SpatialEntity->GetWorldMatrix();
        m_SpawnInWorld = false;
        InitiateSpawnItem(s_PropPair, s_HitmanWorldMatrix);
    }
}

void ChaosEvents::SpawnJohnHitman(EChaosEvent eventRef)
{
    if (!EventJustStarted(eventRef))
        return;
    
    static TEntityRef<ZGlobalOutfitKit>* s_GlobalOutfitKit;
    ZContentKitManager* s_ContentKitManager = Globals::ContentKitManager;
    size_t s_RandomIndex = std::rand() % s_ContentKitManager->m_repositoryGlobalOutfitKits.size();
    int s_CurrentIndex = 0;
    for (auto it = s_ContentKitManager->m_repositoryGlobalOutfitKits.begin(); it != s_ContentKitManager->m_repositoryGlobalOutfitKits.end(); ++it) {
        if (s_CurrentIndex == s_RandomIndex) {
            s_GlobalOutfitKit = &it->second;
        }
        ++s_CurrentIndex;
    }

	static std::string s_RepositoryName = "John Hitman";
    static ZRepositoryID s_RepositoryId = ZRepositoryID("");
    static uint8_t n_CurrentCharacterSetIndex = 0;
    static std::string s_CurrentcharSetCharacterType = "HeroA";
    static uint8_t n_CurrentOutfitVariationIndex = 0;
    
    auto* spawnedActor = SpawnNPC(
        s_RepositoryName,
		s_RepositoryId,
		s_GlobalOutfitKit,
		n_CurrentCharacterSetIndex,
		s_CurrentcharSetCharacterType,
		n_CurrentOutfitVariationIndex
	);

	spawnedActor->m_bStartEnabled = true;

    auto s_LocalHitman = SDK()->GetLocalPlayer();
    auto s_SpatialEntity = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    SMatrix s_HitmanWorldMatrix = s_SpatialEntity->GetWorldMatrix();

    for (int i = 0; i < *Globals::NextActorId; i++)
    {
        auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
        auto& actorName = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef->m_sActorName;
        if (actorName == spawnedActor->m_sActorName)
        {
            Logger::Debug("{} HAS SPAWNED!", actorName);
        }
    }
}

void ChaosEvents::YouGotTheWholeSquadLaughing(EChaosEvent eventRef)
{
	if (!EventJustStarted(eventRef))
		return;

	auto s_LocalHitman = SDK()->GetLocalPlayer();
	auto s_PlayerSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    const auto playerPos = s_PlayerSpatial->GetWorldMatrix().Trans;

    for (int i = 0; i < *Globals::NextActorId; i++)
    {
        auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
        auto actorSpatial = Globals::ActorManager->m_aActiveActors[i].m_ref.QueryInterface<ZSpatialEntity>();
        const auto actorPos = actorSpatial->GetWorldMatrix().Trans;

		// rotate the actor to face the player
        float4 forward = playerPos - actorPos;
        forward.z = 0;
        forward = float4::Norm(forward);

        float4 up = { 0.0f, 0.0f, 1.0f, 0.0f };

        float4 left = float4::CrossProduct(up, forward);
        left = float4::Norm(left);

        up = float4::CrossProduct(forward, left);

        // --- Apply the transform ---
        SMatrix rotationMatrix;
        rotationMatrix.Left = left;
        rotationMatrix.Backward = { -forward.x, -forward.y, -forward.z, forward.w };
        rotationMatrix.Up = up;
        rotationMatrix.Trans = actorPos;

        actorSpatial->SetWorldMatrix(rotationMatrix);
    }
}

void ChaosEvents::TheRotConsumes(EChaosEvent eventRef)
{
    //if (!EventJustStarted(eventRef))
    //    return;

    auto s_LocalHitman = SDK()->GetLocalPlayer();
    auto s_PlayerSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
    const auto playerPos = s_PlayerSpatial->GetWorldMatrix().Trans;

    for (int i = 0; i < *Globals::NextActorId; i++)
    {
        auto& actor = Globals::ActorManager->m_aActiveActors[i].m_pInterfaceRef;
        auto actorSpatial = Globals::ActorManager->m_aActiveActors[i].m_ref.QueryInterface<ZSpatialEntity>();
        const auto actorPos = actorSpatial->GetWorldMatrix().Trans;

        // rotate the actor to face the player
        float4 forward = playerPos - actorPos;
        forward.z = 0;
        forward = float4::Norm(forward);

        float4 up = { 0.0f, 0.0f, 1.0f, 0.0f };

        float4 left = float4::CrossProduct(up, forward);
        left = float4::Norm(left);

        up = float4::CrossProduct(forward, left);

        // --- Apply the transform ---
        SMatrix rotationMatrix;
        rotationMatrix.Left = left;
        rotationMatrix.Backward = { -forward.x, -forward.y, -forward.z, forward.w };
        rotationMatrix.Up = up;
        rotationMatrix.Trans = actorPos;

        actorSpatial->SetWorldMatrix(rotationMatrix);
    }
}