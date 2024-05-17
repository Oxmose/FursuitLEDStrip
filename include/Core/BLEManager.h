/*******************************************************************************
 * @file BLEManager.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/04/2024
 *
 * @version 1.0
 *
 * @brief Bluetooth low energy manager.
 *
 * @details This file provides the bluetooth low energy manager services.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_BLEMANAGER_H_
#define __CORE_BLEMANAGER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cstdint> /* Standard Int Types */
#include <BLEDevice.h> /* BLE Device Services*/
#include <BLEUtils.h>  /* BLE Untils Services*/
#include <BLEServer.h> /* BLE Server Services*/
#include <StripsManager.h> /* Strip Manager service */

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

class BLEManager
{
    /********************* PUBLIC METHODS AND ATTRIBUTES **********************/
    public:
        static BLEManager* GetInstance(void);

        bool ValidateToken(const char* kpToken) const;

        void Update(void);

    /******************* PROTECTED METHODS AND ATTRIBUTES *********************/
    protected:

    /********************* PRIVATE METHODS AND ATTRIBUTES *********************/
    private:
        BLEManager(void);
        void Init(void);
        uint8_t* SerializeStripsInfo(StripsInfoTable_t& rInfoTbl,
                                     size_t& rSize) const;
        uint8_t* SerializePatternsInfo(size_t& rSize) const;

        bool isInit_;

        BLEServer*         pServer_;
        BLEService*        pMainService_;
        BLECharacteristic* pCharacteristicHWersion_;
        BLECharacteristic* pCharacteristicSWersion_;
        BLECharacteristic* pCharacteristicSetToken_;
        BLECharacteristic* pCharacteristicBattery_;
        BLECharacteristic* pCharacteristicBrightness_;
        BLECharacteristic* pCharacteristicGetStrips_;
        BLECharacteristic* pCharacteristicManagePatterns_;
        BLECharacteristic* pCharacteristicManageScenes_;
        BLECharacteristic* pCharacteristicSetScene_;
        BLEAdvertising*    pAdvertising_;

        static BLEManager* PINSTANCE_;
};

#endif /* #ifndef __CORE_BLEMANAGER_H_ */