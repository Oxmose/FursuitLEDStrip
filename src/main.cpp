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

    /* Get the storage */
    psStorage = Storage::GetInstance();
    psStorage->LoadData();

    /* Get the IO Buttons manager instance */
    psIOBtnManager = IOButtonMgr::GetInstance();

    /* Get the System State instance */
    psSysState = SystemState::GetInstance();

    /* Manage boot reason */
    psSysState->ManageBoot();

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

    /* Update the strip manager */
    psStripManager->CheckForActivity();

    /* Update the storage manager without forcing a commit to flash */
    //psStorage->Update(false);

    /* Loop speed down */
    endTime = HWLayer::GetTime();
    if(endTime - startTime < 25000)
    {
        delay((25000 - (endTime - startTime)) / 1000);
    }
}