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

uint16_t StripsManager::AddPattern(const std::shared_ptr<Pattern>& rkNewPattern)
{
    uint16_t newId;

    Lock();
    if(patterns_.count(rkNewPattern->GetId()) != 0)
    {
        Unlock();

        LOG_ERROR("Tried to add existing pattern %d\n", rkNewPattern->GetId());
        return false;
    }

    newId = GetNewPatternId();
    if(newId == 0xFFFF)
    {
        return newId;
    }

    patterns_[newId] = rkNewPattern;
    patterns_[newId]->ForceId(newId);

    Unlock();

    SavePatterns();

    LOG_DEBUG("Added pattern %d\n", rkNewPattern->GetId());

    return newId;
}

bool StripsManager::RemovePattern(const uint16_t kPatternId)
{
    std::unordered_map<uint8_t, uint16_t>::iterator it;
    std::unordered_map<uint8_t, uint16_t>::iterator saveIt;

    Lock();

    if(patterns_.count(kPatternId) == 0)
    {
        Unlock();

        LOG_ERROR("Tried to remove unknown pattern %d\n", kPatternId);
        return false;
    }

    /* Unlink pattern to all scenes */
    for(std::shared_ptr<SScene>& rScene : scenes_)
    {
        for(it = rScene->links.begin(); it != rScene->links.end();)
        {
            if(it->second == kPatternId)
            {
                strips_[it->first]->SetEnabled(false);
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

    CheckForActivity();

    SavePatterns();

    LOG_DEBUG("Erased pattern %d\n", kPatternId);

    return true;
}

bool StripsManager::UpdatePattern(const std::shared_ptr<Pattern>& rkNewPattern)
{
    uint16_t patternId;
    std::unordered_map<uint8_t, uint16_t>::iterator it;

    patternId = rkNewPattern->GetId();

    Lock();

    if(patterns_.count(patternId) == 0)
    {
        Unlock();

        LOG_ERROR("Tried to update unknown pattern %d\n", patternId);
        return false;
    }

    /* Update the pattern */
    patterns_[patternId] = rkNewPattern;

    /* Update colors for all links */
    if(selectedScene_ != 255)
    {
        for(it = scenes_[selectedScene_]->links.begin();
            it != scenes_[selectedScene_]->links.end();
            ++it)
        {
            if(it->second == patternId)
            {
                strips_[it->first]->UpdateColors();
            }
        }
    }

    Unlock();

    CheckForActivity();

    SavePatterns();

    LOG_DEBUG("Updated pattern %d\n", patternId);

    return true;
}

void StripsManager::GetPatternsIds(std::vector<uint16_t>& rPatternIds) const
{
    rPatternIds.clear();

    for(const std::pair<uint16_t, std::shared_ptr<Pattern>>& krPattern : patterns_)
    {
        rPatternIds.push_back(krPattern.first);
    }
}

const Pattern* StripsManager::GetPatternInfo(const uint16_t kPatternId)
{
    if(patterns_.count(kPatternId) != 0)
    {
        return patterns_[kPatternId].get();
    }
    else
    {
        return nullptr;
    }
}

uint16_t StripsManager::GetNewPatternId(void)
{
    uint16_t newId;

    /* Look for available ID */
    for(newId = 0; newId < 0xFFFF; ++newId)
    {
        if(patterns_.count(newId) == 0)
        {
            break;
        }
    }

    if(newId == 0xFFFF)
    {
        LOG_ERROR("No more available pattern ID\n");
    }

    return newId;
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
    scenes_.push_back(rkNewScene);
    retVal = scenes_.size() - 1;
    LOG_DEBUG("Added scene %d\n", scenes_.size() - 1);

    Unlock();

    SaveScenes();

    return retVal;
}

bool StripsManager::RemoveScene(const uint8_t kSceneIdx)
{
    Lock();
    if(kSceneIdx < scenes_.size())
    {
        scenes_.erase(scenes_.begin() + kSceneIdx);

        if(scenes_.size() != 0)
        {
            /* Was selected */
            if(selectedScene_ == kSceneIdx)
            {
                selectedScene_ = scenes_.size() - 1;
            }
            else if(selectedScene_ > kSceneIdx)
            {
                --selectedScene_;
            }

            Unlock();
            ActivateScene();
        }
        else
        {
            selectedScene_ = 255;
            Unlock();
        }

        CheckForActivity();

        SaveScenes();
        SaveSelectedScene();

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
    if(kSceneIdx < scenes_.size())
    {
        scenes_[kSceneIdx] = rkScene;
        if(selectedScene_ == kSceneIdx)
        {
            Unlock();
            ActivateScene();
            return true;
        }

        LOG_DEBUG("Updated scene %d\n", kSceneIdx);
        Unlock();

        CheckForActivity();

        SaveScenes();

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
    if(kSceneIdx < scenes_.size())
    {
        selectedScene_ = kSceneIdx;
        ActivateScene();
        SystemState::GetInstance()->NotifyUpdate();

        CheckForActivity();

        SaveSelectedScene();

        LOG_DEBUG("Updated selected scene %d\n", kSceneIdx);
    }
    else
    {
        LOG_ERROR("Tried to select unknown scene %d\n", kSceneIdx);
    }
}

uint8_t StripsManager::GetSelectedScene(void) const
{
    return selectedScene_;
}

const SScene* StripsManager::GetSceneInfo(const uint8_t kSceneId)
{
    if(kSceneId < scenes_.size())
    {
        return scenes_[kSceneId].get();
    }
    else
    {
        return nullptr;
    }
}

uint8_t StripsManager::GetSceneCount(void) const
{
    return (uint8_t)scenes_.size();
}

void StripsManager::CheckForActivity(void)
{
    bool     hasEnabled;
    uint16_t patternId;

    hasEnabled = false;

    Lock();

    /* Check if current scene is 255 or current brightness is 0 or there is
     * no link
     */
    if(selectedScene_ == 255 ||
       SystemState::GetInstance()->GetBrightness() == 0 ||
       scenes_[selectedScene_]->links.size() == 0)
    {
        Unlock();
        Disable();
        return;
    }

    /* For all links, check if patterns brightness is greater than 0 */
    for(const std::pair<uint8_t, std::shared_ptr<LEDStrip>>& krStrip : strips_)
    {
        if(scenes_[selectedScene_]->links.count(krStrip.first) != 0)
        {
            patternId = scenes_[selectedScene_]->links[krStrip.first];
            if(patterns_[patternId]->GetBrightness() == 0)
            {
                krStrip.second->SetEnabled(false);
            }
            else
            {
                hasEnabled = true;
            }
        }
        else
        {
            krStrip.second->SetEnabled(false);
        }
    }

    Unlock();

    /* Check if all strips are disabled */
    if(hasEnabled == false)
    {
        Disable();
    }
    else
    {
        Enable();
    }
}

void StripsManager::Enable(void)
{
    if(isEnabled_ == true)
    {
        return;
    }

    LOG_DEBUG("Enabling Strip Manager\n");

    /* Enable the worker thread */
    vTaskResume(workerThread_);

    isEnabled_ = true;
}

void StripsManager::Disable(void)
{
    if(isEnabled_ == false)
    {
        return;
    }

    LOG_DEBUG("Disabling Strip Manager\n");

    /* Disable the worker thread */
    xSemaphoreTake(threadWorkLock_, portMAX_DELAY);

    vTaskSuspend(workerThread_);

    xSemaphoreGive(threadWorkLock_);

    isEnabled_ = false;
}

void StripsManager::Kill(void)
{
    Disable();
    for(const std::pair<uint8_t, std::shared_ptr<LEDStrip>>& rStrip : strips_)
    {
        rStrip.second->SetEnabled(false);
    }
}

void StripsManager::Lock(void)
{
    xSemaphoreTake(managerLock_, portMAX_DELAY);
}

void StripsManager::Unlock(void)
{
    xSemaphoreGive(managerLock_);
}

StripsManager::StripsManager(void)
{
    Storage* pStorage;
    std::vector<std::shared_ptr<Pattern>> patterns;

    isEnabled_ = true;

    /* Init locks */
    managerLock_    = xSemaphoreCreateBinary();
    threadWorkLock_ = xSemaphoreCreateMutex();
    xSemaphoreGive(managerLock_);

    /* Add Cross/ strip */
    AddStrip(std::make_shared<LEDStripC<GPIO_NUM_4, GPIO_NUM_6, 120>>("Cross/"));
    AddStrip(std::make_shared<LEDStripC<GPIO_NUM_5, GPIO_NUM_7, 70>>("Cross\\"));

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
                            "LEDWorker",
                            4096,
                            this,
                            0,
                            &workerThread_,
                            1);

    LOG_INFO("Strip Manager Initialized.\n");
}

void StripsManager::AddStrip(std::shared_ptr<LEDStrip> newStrip)
{
    strips_[newStrip->GetId()] = newStrip;
    LOG_DEBUG("Added new strip %s.\n", newStrip->GetName().c_str())
}

void StripsManager::ActivateScene(void)
{
    std::unordered_map<uint8_t, std::shared_ptr<LEDStrip>>::iterator it;

    Lock();

    /* Check for active scene */
    if(selectedScene_ == 255)
    {
        Unlock();
        return;
    }

    /* Update colors for all strips in the scene */
    for(it = strips_.begin(); it != strips_.end(); ++it)
    {
        if(scenes_[selectedScene_]->links.count(it->first) == 0)
        {
            it->second->SetEnabled(false);
        }
        else
        {
            it->second->SetEnabled(true);
            it->second->UpdateColors();
        }
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
        if(pManager->selectedScene_ != 255)
        {
            const std::unordered_map<uint8_t, uint16_t>& krLinks =
                pManager->scenes_[pManager->selectedScene_]->links;
            for(it = krLinks.begin(); it != krLinks.end(); ++it)
            {
                if(it->second != NO_PATTERN)
                {
                    /* Apply linked patterns */
                    pManager->strips_[it->first]->Apply(pManager->patterns_[it->second].get());
                }
            }
        }
        else
        {
            FastLED.setBrightness(0);
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

void StripsManager::SavePatterns(void)
{
    std::vector<std::shared_ptr<Pattern>> savedPatterns;

    Lock();

    for(const std::pair<uint16_t, std::shared_ptr<Pattern>>& krPattern : patterns_)
    {
        savedPatterns.push_back(krPattern.second);
    }

    Unlock();

    Storage::GetInstance()->SavePatterns(savedPatterns);
}

void StripsManager::SaveScenes(void)
{
    std::vector<std::shared_ptr<SScene>> savedScenes;

    Lock();

    savedScenes = scenes_;

    Unlock();

    Storage::GetInstance()->SaveScenes(savedScenes);
}

void StripsManager::SaveSelectedScene(void) const
{
    Storage::GetInstance()->SaveSelectedScene(selectedScene_);
}