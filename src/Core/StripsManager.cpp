/*******************************************************************************
 * @file StripsManager.cpp
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/04/2024
 *
 * @version 1.0
 *
 * @brief LED strips manager.
 *
 * @details This file provides the LED strips manager services.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <cstdint>    /* Standard Int Types */
#include <vector>     /* std::vector */
#include <memory>     /* std::shared_ptr */
#include <unordered_map> /* std::unordered_map */
#include <Types.h>    /* Defined types */
#include <LEDStrip.hpp> /* LED strip driver*/
#include <Logger.h> /* Logger services */
#include <FastLED.h> /* FastLED driver */
#include <Storage.h> /* Storage service */
#include <SystemState.h> /* SystemState services */
#include <HWLayer.h>    /* HW Layer abstraction */
#include <Arduino.h>     /* Semaphore services */

/* Header File */
#include <StripsManager.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define NO_PATTERN              0xFFFF
#define UPDATE_ROUTINE_DELAY_US 10000

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
StripsManager* StripsManager::PINSTANCE_ = nullptr;

/************************** Static global variables ***************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CLASS METHODS
 ******************************************************************************/

StripsManager* StripsManager::GetInstance(void)
{
    if (StripsManager::PINSTANCE_ == nullptr)
    {
        StripsManager::PINSTANCE_ = new StripsManager();
    }

    return StripsManager::PINSTANCE_;
}

void StripsManager::AddStrip(std::shared_ptr<LEDStrip> newStrip)
{
    strips_[newStrip->GetId()] = newStrip;
    LOG_DEBUG("Added new strip %s.\n", newStrip->GetName().c_str())
}

void StripsManager::GetStripsInfo(StripsInfoTable_t& rStripsInfo) const
{
    std::unordered_map<uint8_t, std::shared_ptr<LEDStrip>>::const_iterator it;

    rStripsInfo.clear();
    LOG_DEBUG("Number of strips: %d.\n", strips_.size());
    for(it = strips_.begin(); it != strips_.end(); ++it)
    {
        LOG_DEBUG("Reading info for %s.\n", it->second->GetName().c_str());
        std::shared_ptr<SStripInfo> info = std::make_shared<SStripInfo>();
        it->second->GetStripInfo(info);
        rStripsInfo.push_back(info);
    }
}

void StripsManager::AddPattern(const std::shared_ptr<Pattern>& rkNewPattern)
{
    patterns_[rkNewPattern->GetId()] = rkNewPattern;
    LOG_DEBUG("Added pattern %d\n", rkNewPattern->GetId());
}

void StripsManager::RemovePattern(const uint16_t kPatternId)
{
    std::unordered_map<uint8_t, uint16_t>::iterator it;
    std::unordered_map<uint8_t, uint16_t>::iterator saveIt;

    Lock();

    /* Unlink pattern to all scenes */
    for(std::shared_ptr<SScene>& rScene : scenes_.scenes)
    {
        for(it = rScene->links.begin(); it != rScene->links.end();)
        {
            if(it->second == kPatternId)
            {
                it = rScene->links.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    /* Remove pattern */
    patterns_.erase(kPatternId);

    Unlock();

    LOG_DEBUG("Erased pattern %d\n", kPatternId);
}

void StripsManager::UpdatePattern(const std::shared_ptr<Pattern>& rkNewPattern)
{
    uint16_t patternId;
    std::unordered_map<uint8_t, uint16_t>::iterator it;

    patternId = rkNewPattern->GetId();

    Lock();

    /* Update the pattern */
    patterns_[patternId] = rkNewPattern;

    /* Update colors for all links */
    for(it = scenes_.scenes[scenes_.selectedIdx]->links.begin();
        it != scenes_.scenes[scenes_.selectedIdx]->links.end();
        ++it)
    {
        if(it->second == patternId)
        {
            strips_[it->first]->UpdateColors();
        }
    }

    Unlock();

    LOG_DEBUG("Updated pattern %d\n", patternId);
}

uint8_t StripsManager::AddScene(const std::shared_ptr<SScene>& rkNewScene)
{
    uint8_t retVal;

    Lock();

    /* Check that the patterns and strips exist for the scene */
    for(const std::pair<uint8_t, uint16_t>& link : rkNewScene->links)
    {
        if(strips_.count(link.first) == 0 ||
           patterns_.count(link.second) == 0)
        {
            LOG_ERROR("Tried to add scene with unknown strip or pattern\n");
            Unlock();
            return -1;
        }
    }

    /* Add the scene */
    scenes_.scenes.push_back(rkNewScene);
    retVal = scenes_.scenes.size() - 1;
    LOG_DEBUG("Added scene %d\n", scenes_.scenes.size() - 1);

    Unlock();

    return retVal;
}

bool StripsManager::RemoveScene(const uint8_t kSceneIdx)
{
    Lock();
    if(kSceneIdx < scenes_.scenes.size())
    {
        scenes_.scenes.erase(scenes_.scenes.begin() + kSceneIdx);

        if(scenes_.scenes.size() != 0)
        {
            /* Was selected */
            if(scenes_.selectedIdx == kSceneIdx)
            {
                if(scenes_.scenes.size() != 0)
                {
                    scenes_.selectedIdx = scenes_.scenes.size() - 1;
                }

            }
            else if(scenes_.selectedIdx > kSceneIdx)
            {
                --scenes_.selectedIdx;
            }

            Unlock();
            ActivateScene();
        }
        else
        {
            scenes_.selectedIdx = -1;
            Unlock();
        }

        LOG_DEBUG("Removed scene %d\n", kSceneIdx);
        return true;
    }
    else
    {
        LOG_ERROR("Tried to remove unknown scene %d\n", kSceneIdx);
        Unlock();
        return false;
    }
}

bool StripsManager::UpdateScene(const uint8_t kSceneIdx,
                                const std::shared_ptr<SScene>& rkScene)
{
    Lock();
    if(kSceneIdx < scenes_.scenes.size())
    {
        scenes_.scenes[kSceneIdx] = rkScene;
        if(scenes_.selectedIdx == kSceneIdx)
        {
            Unlock();
            ActivateScene();
            return true;
        }

        LOG_DEBUG("Updated scene %d\n", kSceneIdx);
        Unlock();
        return true;
    }
    else
    {
        LOG_ERROR("Tried to update unknown scene %d\n", kSceneIdx);
        Unlock();
        return false;
    }
}

void StripsManager::SelectScene(const uint8_t kSceneIdx)
{
    if(kSceneIdx < scenes_.scenes.size())
    {
        scenes_.selectedIdx = kSceneIdx;
        ActivateScene();
        SystemState::GetInstance()->NotifyUpdate();
        LOG_DEBUG("Updated selected scene %d\n", kSceneIdx);
    }
    else
    {
        LOG_ERROR("Tried to select unknown scene %d\n", kSceneIdx);
    }
}

uint8_t StripsManager::GetSelectedScene(void) const
{
    return scenes_.selectedIdx;
}

const SScene* StripsManager::GetSceneInfo(const uint8_t kSceneId)
{
    if(kSceneId < scenes_.scenes.size())
    {
        return scenes_.scenes[kSceneId].get();
    }
    else
    {
        return nullptr;
    }
}

uint8_t StripsManager::GetSceneCount(void) const
{
    return (uint8_t)scenes_.scenes.size();
}

StripsManager::StripsManager(void)
{
    Storage* pStorage;
    std::vector<std::shared_ptr<Pattern>> patterns;

    /* Init locks */
    managerLock_    = xSemaphoreCreateBinary();
    threadWorkLock_ = xSemaphoreCreateMutex();
    xSemaphoreGive(managerLock_);

    /* Add Cross/ strip */
    AddStrip(std::make_shared<LEDStripC<GPIO_NUM_18, 120>>("Cross/", true));
    AddStrip(std::make_shared<LEDStripC<GPIO_NUM_19, 70>>("Cross\\", true));

    pStorage = Storage::GetInstance();

    /* Get patterns from storage */
    pStorage->GetPatterns(patterns);
    for(const std::shared_ptr<Pattern>& krPattern : patterns)
    {
        patterns_.emplace(krPattern->GetId(), krPattern);
    }

    /* Get scenes from storage */
    pStorage->GetScenes(scenes_);
    ActivateScene();

    /* Start worker thread */
    xTaskCreatePinnedToCore(UpdateRoutine,
                            "LEDManagerWorker",
                            2048,
                            this,
                            0,
                            &workerThread_,
                            1);

    LOG_INFO("Strip Manager Initialized.\n");
}

void StripsManager::Lock(void)
{
    xSemaphoreTake(managerLock_, portMAX_DELAY);
}

void StripsManager::Unlock(void)
{
    xSemaphoreGive(managerLock_);
}

void StripsManager::ActivateScene(void)
{
    std::unordered_map<uint8_t, uint16_t>::iterator it;

    Lock();

    /* Update colors for all strips in the scene */
    for(it = scenes_.scenes[scenes_.selectedIdx]->links.begin();
        it != scenes_.scenes[scenes_.selectedIdx]->links.end();
        ++it)
    {
        strips_[it->first]->UpdateColors();
    }

    Unlock();
}

void StripsManager::UpdateRoutine(void* objThis)
{
    uint64_t       startTime;
    uint64_t       diffTime;
    StripsManager* pManager;
    std::unordered_map<uint8_t, uint16_t>::const_iterator it;

    pManager = (StripsManager*)objThis;

    LOG_DEBUG("Worker thread on core %d\n", xPortGetCoreID());

    while(1)
    {
        startTime = HWLayer::GetTime();
        xSemaphoreTake(pManager->threadWorkLock_, portMAX_DELAY);
        /* Update general brightness */
        FastLED.setBrightness(SystemState::GetInstance()->GetBrightness());

        pManager->Lock();
        const std::unordered_map<uint8_t, uint16_t>& krLinks =
            pManager->scenes_.scenes[pManager->scenes_.selectedIdx]->links;
        for(it = krLinks.begin(); it != krLinks.end(); ++it)
        {
            if(it->second != NO_PATTERN && pManager->strips_[it->first]->IsEnabled())
            {
                /* Apply linked patterns */
                pManager->strips_[it->first]->Apply(pManager->patterns_[it->second].get());
            }
        }
        pManager->Unlock();

        /* Show and delay */
        FastLED.show();
        xSemaphoreGive(pManager->threadWorkLock_);
        diffTime = HWLayer::GetTime() - startTime;

        if(diffTime < UPDATE_ROUTINE_DELAY_US)
        {
            FastLED.delay((UPDATE_ROUTINE_DELAY_US - diffTime) / 1000);
        }
    }
}