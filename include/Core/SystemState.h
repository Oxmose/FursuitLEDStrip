/*******************************************************************************
 * @file SystemState.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 20/04/2024
 *
 * @version 1.0
 *
 * @brief This file prvides the system state service.
 *
 * @details This file provides the system state service. This files defines
 * the different features embedded in the system state.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_SYSTEM_STATE_H_
#define __CORE_SYSTEM_STATE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cstdint> /* Standard Int Types */
#include <OLED.h>  /* OLED screen manager */
#include <IOButtonMgr.h> /* Button manager */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define BLE_PIN_SIZE_MAX 8
#define BLE_TOCKEN_SIZE  16

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

typedef enum
{
    SYS_IDLE   = 0,
    SYS_MENU_0 = 1,
    SYS_MENU_1 = 2,
} ESystemState;

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

class SystemState
{
    /********************* PUBLIC METHODS AND ATTRIBUTES **********************/
    public:
        static SystemState* GetInstance(void);

        void SetBatteryPercent(const uint8_t kNewBatteryPercent);
        uint8_t GetBatteryPercent(void) const;

        void SetBrightness(const uint8_t kNewBrightness);
        uint8_t GetBrightness(void) const;

        const char* GetBLEToken(void) const;
        void SetBLEToken(const char* kpNewToken);

        const char* GetBLEPin(void) const;
        void SetBLEPin(const char* kpNewPin);

        void Update(void);
        void NotifyUpdate(void);
        void ManageBoot(void);

    /******************* PROTECTED METHODS AND ATTRIBUTES *********************/
    protected:

    /********************* PRIVATE METHODS AND ATTRIBUTES *********************/
    private:
        SystemState(void);

        void Init(void);

        void UpdateState(void);
        void SetSystemState(const ESystemState kNewState);
        void ManageIdle(void);
        void ManageMenu0(void);
        void ManageMenu1(void);

        void Hibernate(const bool kDisplay);

        uint8_t  batteryPercent_;
        uint8_t  currentBrightness_;

        uint64_t lastEventTime_;

        char pCurrentBLEPIN_[BLE_PIN_SIZE_MAX + 1];
        char pCurrentBLEToken_[BLE_TOCKEN_SIZE + 1];

        OLED oledDisplay_;
        bool displayNeedUpdate_;

        uint64_t     pButtonsKeepTime_[EButtonID::BUTTON_MAX_ID];
        EButtonState pButtonsState_[EButtonID::BUTTON_MAX_ID];
        EButtonState pPrevButtonsState_[EButtonID::BUTTON_MAX_ID];

        ESystemState currentState_;
        ESystemState previousState_;

        static SystemState* PINSTANCE_;
};

#endif /* #ifndef __CORE_SYSTEM_STATE_H_ */