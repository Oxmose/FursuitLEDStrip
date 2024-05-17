/*******************************************************************************
 * @file IOButtonMgr.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 18/12/2022
 *
 * @version 1.0
 *
 * @brief This file contains the IO buttons manager.
 *
 * @details This file contains the IO buttons manager. The file provides the
 * services read input buttons and associate interrupts to the desired pins.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __BSP_IOBUTTONMGR_H_
#define __BSP_IOBUTTONMGR_H_

/****************************** OUTER NAMESPACE *******************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <string>  /* std::string */
#include <cstdint> /* Generic Types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/****************************** INNER NAMESPACE *******************************/

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

typedef enum
{
    BTN_STATE_UP   = 0,
    BTN_STATE_DOWN = 1,
    BTN_STATE_KEEP = 2
} EButtonState;

typedef enum
{
    BUTTON_ENTER = 0,
    BUTTON_BOOT  = BUTTON_ENTER,
    BUTTON_MAX_ID
} EButtonID;

typedef enum
{
    ENTER_PIN = 13,
    BOOT_PIN  = ENTER_PIN
} EButtonPin;

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

class IOButtonMgr
{
    /********************* PUBLIC METHODS AND ATTRIBUTES **********************/
    public:
        static IOButtonMgr* GetInstance(void);

        void Update(void);

        EButtonState GetButtonState(const EButtonID kBtnId) const;
        uint64_t     GetButtonKeepTime(const EButtonID kBtnId) const;

    /******************* PROTECTED METHODS AND ATTRIBUTES *********************/
    protected:

    /********************* PRIVATE METHODS AND ATTRIBUTES *********************/
    private:
        IOButtonMgr(void);
        void Init(void);

        void SetupBtn (const EButtonID  kBtnId, const EButtonPin kBtnPin);

        int8_t       pBtnPins_[BUTTON_MAX_ID];
        uint64_t     pBtnLastPress_[BUTTON_MAX_ID];
        EButtonState pBtnStates_[BUTTON_MAX_ID];

        /* Instance */
        static IOButtonMgr* PINSTANCE_;
};


#endif /* #ifndef __BSP_IOBUTTONMGR_H_ */