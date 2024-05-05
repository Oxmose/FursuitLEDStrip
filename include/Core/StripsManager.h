/*******************************************************************************
 * @file StripsManager.h
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

#ifndef __CORE_STRIPS_MANAGER_H_
#define __CORE_STRIPS_MANAGER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cstdint>    /* Standard Int Types */
#include <vector>     /* std::vector */
#include <memory>     /* std::shared_ptr */
#include <unordered_map> /* std::unordered_map */
#include <Types.h>    /* Defined types */
#include <LEDStrip.hpp> /* LED strip driver*/
#include <Pattern.h>  /* Pattern object */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

typedef std::vector<std::shared_ptr<SStripInfo>> StripsInfoTable_t;

typedef struct
{
    std::string                           name;
    std::unordered_map<uint8_t, uint16_t> links;
} SScene;

typedef struct
{
    std::vector<std::shared_ptr<SScene>> scenes;
    uint8_t                              selectedIdx;
} SSceneTable;


/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

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
 * CLASSES
 ******************************************************************************/

class StripsManager
{
    /********************* PUBLIC METHODS AND ATTRIBUTES **********************/
    public:
        static StripsManager* GetInstance(void);

        void AddStrip(std::shared_ptr<LEDStrip> newStrip);
        void GetStripsInfo(StripsInfoTable_t& rStripsInfo) const;

        void AddPattern(const std::shared_ptr<Pattern>& rkNewPattern);
        void RemovePattern(const uint16_t kPatternId);
        void UpdatePattern(const std::shared_ptr<Pattern>& rkNewPattern);
        const Pattern* GetPatternInfo(const uint8_t kPatternId);
        void GetPatternsIds(std::vector<uint8_t> rPatternIds) const;

        uint8_t AddScene(const std::shared_ptr<SScene>& rkNewScene);
        bool RemoveScene(const uint8_t kSceneIdx);
        bool UpdateScene(const uint8_t kSceneIdx,
                         const std::shared_ptr<SScene>& rkScene);
        void SelectScene(const uint8_t kSceneIdx);
        uint8_t GetSelectedScene(void) const;
        const SScene* GetSceneInfo(const uint8_t kSceneId);
        uint8_t GetSceneCount(void) const;

        void Lock(void);
        void Unlock(void);

    /******************* PROTECTED METHODS AND ATTRIBUTES *********************/
    protected:

    /********************* PRIVATE METHODS AND ATTRIBUTES *********************/
    private:
        StripsManager(void);

        void ActivateScene(void);

        static void UpdateRoutine(void* objThis);


        std::unordered_map<uint8_t, std::shared_ptr<LEDStrip>> strips_;
        std::unordered_map<uint16_t, std::shared_ptr<Pattern>> patterns_;
        SSceneTable                                            scenes_;

        SemaphoreHandle_t threadWorkLock_;
        SemaphoreHandle_t managerLock_;

        TaskHandle_t workerThread_;

        static StripsManager* PINSTANCE_;
};

#endif /* #ifndef __CORE_STRIPS_MANAGER_H_ */