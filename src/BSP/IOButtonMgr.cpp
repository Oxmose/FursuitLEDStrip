/*******************************************************************************
 * @file IOButtonMgr.cpp
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <cstring>   /* String manipulation*/
#include <Types.h>   /* Defined Types */
#include <Logger.h>  /* Logger service */
#include <Arduino.h> /* Arduino service */
#include <HWLayer.h> /* HW Layer service */

/* Header File */
#include <IOButtonMgr.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define BTN_KEEP_WAIT_TIME 1000000 /* us: 1s */

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
IOButtonMgr* IOButtonMgr::PINSTANCE_ = nullptr;

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

IOButtonMgr* IOButtonMgr::GetInstance(void)
{
    if (IOButtonMgr::PINSTANCE_ == nullptr)
    {
        IOButtonMgr::PINSTANCE_ = new IOButtonMgr();
    }

    return IOButtonMgr::PINSTANCE_;
}


EErrorCode IOButtonMgr::Update(void)
{
    uint8_t  i;
    uint8_t  btnState;
    uint64_t currTime;

    /* Check all pins */
    for(i = 0; i < BUTTON_MAX_ID; ++i)
    {
        if(pBtnPins_[i] != -1)
        {
            btnState = digitalRead(pBtnPins_[i]);

            if(btnState != 0)
            {
                currTime = HWLayer::GetTime();
                /* If this is the first time the button is pressed */
                if(pBtnStates_[i] == BTN_STATE_UP)
                {
                    pBtnStates_[i]    = BTN_STATE_DOWN;
                    pBtnLastPress_[i] = currTime;
                }
                else if(currTime - pBtnLastPress_[i] > BTN_KEEP_WAIT_TIME)
                {
                    pBtnStates_[i] = BTN_STATE_KEEP;
                }
            }
            else
            {
                /* When the button is released, its state is allways UP */
                pBtnStates_[i] = BTN_STATE_UP;
            }
        }
    }
    return NO_ERROR;
}

EButtonState IOButtonMgr::GetButtonState(const EButtonID kBtnId) const
{
    if(kBtnId < BUTTON_MAX_ID)
    {
        return pBtnStates_[kBtnId];
    }

    return BTN_STATE_DOWN;
}

uint64_t IOButtonMgr::GetButtonKeepTime(const EButtonID kBtnId) const
{
    if(kBtnId < BUTTON_MAX_ID &&
       pBtnStates_[kBtnId] == BTN_STATE_KEEP)
    {
        return HWLayer::GetTime() - pBtnLastPress_[kBtnId];
    }
    return 0;
}

IOButtonMgr::IOButtonMgr(void)
{
    /* Init pins and handlers */
    memset(pBtnPins_, -1, sizeof(int8_t) * BUTTON_MAX_ID);
    memset(pBtnLastPress_, 0, sizeof(uint64_t) * BUTTON_MAX_ID);
    memset(pBtnStates_, 0, sizeof(EButtonState) * BUTTON_MAX_ID);

    if(Init() != NO_ERROR)
    {
        LOG_ERROR("Failed to initialize the IO buttons\n");
    }
    else
    {
        LOG_INFO("IO Button Manager Initialized.\n")
    }
}

EErrorCode IOButtonMgr::Init(void)
{
    EErrorCode retCode;

    retCode = SetupBtn(BUTTON_ENTER, ENTER_PIN);
    if(retCode != NO_ERROR)
    {
        LOG_ERROR("Failed to init ENTER. Error %d\n", retCode);
        return retCode;
    }

    return retCode;
}

EErrorCode IOButtonMgr::SetupBtn(const EButtonID kBtnId, const EButtonPin kBtnPin)
{
    EErrorCode retCode;

    if(kBtnId < BUTTON_MAX_ID)
    {
        pinMode(kBtnPin, INPUT);
        pBtnPins_[kBtnId] = kBtnPin;

        retCode = NO_ERROR;
    }
    else
    {
        retCode = INVALID_PARAM;
    }

    return retCode;
}