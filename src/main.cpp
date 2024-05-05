/*******************************************************************************
 * @file main.cpp
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 20/04/2024
 *
 * @version 1.0
 *
 * @brief ESP32 Main entry point file.
 *
 * @details ESP32 Main entry point file. This file contains the main functions
 * of the ESP32 software module.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <cstdint>   /* Generic types */
#include <Arduino.h> /* Arduino Main Header File */
#include <HWLayer.h> /* Hardware services */
#include <Types.h>   /* Custom types */
#include <Logger.h>  /* Logger */
#include <version.h> /* Versioning */
#include <SystemState.h> /* System state */
#include <BLEManager.h> /* BLE Manager */
#include <StripsManager.h> /* Strips manager */
#include <Storage.h>       /* Storage manager */
#include <IOButtonMgr.h>  /* IO buttons manager */
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

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
static SystemState*   psSysState;
static BLEManager*    psBLEManager;
static StripsManager* psStripManager;
static Storage*       psStorage;
static IOButtonMgr*   psIOBtnManager;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Setup function of the ESP32 software module.
 *
 * @details Setup function of the ESP32 software module. This function is used
 * to initialize the module's state and hardware. It is only called once on
 * board's or CPU reset.
 */
void setup(void);

/**
 * @brief Main execution loop of the ESP32 software module.
 *
 * @details Main execution loop of the ESP32 software module. This function
 * never returns and performs the main loop of the ESP32 software module.
 */
void loop(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
#define INIT_FLASH 0
#if INIT_FLASH
static void InitFlash(void)
{
    Storage* pStorage;
    std::shared_ptr<Pattern> patternPtr0;
    std::shared_ptr<Pattern> patternPtr1;
    std::shared_ptr<Pattern> patternPtr2;
    std::vector<std::shared_ptr<Pattern>> pat;
    std::vector<SColor> colors;
    std::vector<SAnimation> anims;
    std::unordered_map<uint8_t, uint16_t> map;
    SSceneTable scenes;

    pStorage = Storage::GetInstance();

    patternPtr0 = std::make_shared<Pattern>(0, "P0");
    patternPtr0->SetBrightness(25);
    colors.push_back({
        .startIdx = 14,
        .endIdx   = 29,
        .startColorCode = 255,
        .endColorCode = 0
    });
    colors.push_back({
        .startIdx = 44,
        .endIdx   = 59,
        .startColorCode = 255,
        .endColorCode = 0
    });
    colors.push_back({
        .startIdx = 60,
        .endIdx   = 75,
        .startColorCode = 0,
        .endColorCode = 255
    });
    colors.push_back({
        .startIdx = 90,
        .endIdx   = 105,
        .startColorCode = 0,
        .endColorCode = 255
    });
    patternPtr0->SetColors(colors);

    anims.push_back({
        .type = ANIM_TRAIL,
        .startIdx = 0,
        .endIdx = 59,
        .param = 1
    });
    anims.push_back({
        .type = ANIM_TRAIL,
        .startIdx = 119,
        .endIdx = 60,
        .param = 1
    });
    patternPtr0->SetAnimations(anims);

    patternPtr1 = std::make_shared<Pattern>(1, "P1");
    patternPtr1->SetBrightness(25);
    colors.clear();
    colors.push_back({
        .startIdx = 0,
        .endIdx   = 69,
        .startColorCode = 255 << 16,
        .endColorCode = 0
    });
    patternPtr1->SetColors(colors);

    anims.clear();
    anims.push_back({
        .type = ANIM_BREATH,
        .startIdx = 0,
        .endIdx = 34,
        .param = 1
    });

    patternPtr1->SetAnimations(anims);

    patternPtr2 = std::make_shared<Pattern>(2, "P2");
    patternPtr2->SetBrightness(100);
    colors.clear();
    colors.push_back({
        .startIdx = 0,
        .endIdx   = 69,
        .startColorCode = 255 << 8,
        .endColorCode = 0
    });
    patternPtr2->SetColors(colors);

    anims.push_back({
        .type = ANIM_TRAIL,
        .startIdx = 0,
        .endIdx = 20,
        .param = 1
    });
    patternPtr2->SetAnimations(anims);

    /* Patterns */
    pat.push_back(patternPtr0);
    pat.push_back(patternPtr1);
    pat.push_back(patternPtr2);

    /* Scenes */
    scenes.selectedIdx = 1;
    scenes.scenes.push_back(std::make_shared<SScene>());
    scenes.scenes.push_back(std::make_shared<SScene>());

    scenes.scenes[0]->name = "Scene0";
    scenes.scenes[0]->links.emplace(18, 0);
    scenes.scenes[0]->links.emplace(19, 1);

    scenes.scenes[1]->name = "Scene1";
    scenes.scenes[1]->links.emplace(18, 2);
    scenes.scenes[1]->links.emplace(19, 2);


    pStorage->SaveBrightness(255);
    pStorage->SavePin("0000");
    pStorage->SaveToken("1234567891113150");
    pStorage->SavePatterns(pat);
    pStorage->SaveScenes(scenes);

    pStorage->Update(true);
}
#endif

void setup(void)
{
    char uniqueHWUID[HW_ID_LENGTH + 1];

    /* Init logger */
    INIT_LOGGER(LOG_LEVEL_DEBUG, false);

    /* Get the unique hardware ID */
    strncpy(uniqueHWUID, HWLayer::GetHWUID(), HW_ID_LENGTH);
    uniqueHWUID[HW_ID_LENGTH] = 0;

    LOG_INFO("#========================#\n");
    LOG_INFO("| HWUID: %s    |\n", uniqueHWUID);
    LOG_INFO("| MAC: %s |\n", HWLayer::GetMacAddress())
    LOG_INFO("#========================#\n");
    LOG_INFO("===> SW " VERSION "\n");


#if INIT_FLASH
    InitFlash();
#endif

    /* Get the IO Buttons manager instance */
    psIOBtnManager = IOButtonMgr::GetInstance();

    /* Get the System State instance */
    psSysState = SystemState::GetInstance();

    /* Manage boot reason */
    psSysState->ManageBoot();

    /* Get the storage */
    psStorage = Storage::GetInstance();

    /* Setup the strip manager */
    psStripManager = StripsManager::GetInstance();

    /* Get the BLE manager instance */
    psBLEManager = BLEManager::GetInstance();
}

void loop(void)
{
    uint64_t startTime;
    uint64_t endTime;

    startTime = HWLayer::GetTime();

    /* Update buttons state */
    psIOBtnManager->Update();

    /* Update the system state */
    psSysState->Update();

    /* Update the BLE manager */
    psBLEManager->Update();

    /* Update the storage manager without forcing a commit to flash */
    //psStorage->Update(false);

    /* Loop speed down */
    endTime = HWLayer::GetTime();
    if(endTime - startTime < 25000)
    {
        delay((25000 - (endTime - startTime)) / 1000);
    }
}