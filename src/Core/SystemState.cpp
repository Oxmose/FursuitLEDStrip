/*******************************************************************************
 * @file SystemState.cpp
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/04/2024
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <OLED.h>    /* OLED screen manager */
#include <cstring>   /* String manipulation*/
#include <Logger.h>  /* Logging service */
#include <Arduino.h> /* Arduino Services */
#include <version.h> /* Versioning */
#include <HWLayer.h> /* Hardware layer */
#include <Storage.h> /* Storage service */
#include <IOButtonMgr.h> /* Button manager */

/* Header File */
#include <SystemState.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define SYSTEM_IDLE_TIME     15000000 /* us : 15 sec*/
#define HIBER_BTN_PRESS_TIME 3000000  /* us : 3 sec*/
#define MENU_BTN_PRESS_TIME  1000000  /* us : 1 sec */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

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
SystemState* SystemState::PINSTANCE_ = nullptr;

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
SystemState* SystemState::GetInstance(void)
{
    if (SystemState::PINSTANCE_ == nullptr)
    {
        SystemState::PINSTANCE_ = new SystemState();
    }

    return SystemState::PINSTANCE_;
}

void SystemState::Update(void)
{
    /* Update the state */
    UpdateState();

    /* Manage the state  */
    switch(currentState_)
    {
        case SYS_IDLE:
            ManageIdle();
            break;
        case SYS_MENU_0:
            ManageMenu0();
            break;
        case SYS_MENU_1:
            ManageMenu1();
            break;
        default:
            LOG_ERROR("Unknown state %d\n", currentState_);
    }
}

void SystemState::NotifyUpdate(void)
{
    displayNeedUpdate_ = true;
}

void SystemState::ManageBoot(void)
{
    esp_sleep_wakeup_cause_t wakeupReason;
    EButtonState             btState;
    IOButtonMgr*             pButtonMgr_;

    pButtonMgr_ = IOButtonMgr::GetInstance();

    wakeupReason = esp_sleep_get_wakeup_cause();

    LOG_DEBUG("Boot reason: %d\n", wakeupReason);

    /* On normal boot, return */
    if(wakeupReason != ESP_SLEEP_WAKEUP_EXT1)
    {
        Init();
        return;
    }

    do
    {
        pButtonMgr_->Update();
        btState = pButtonMgr_->GetButtonState(BUTTON_BOOT);

        if(pButtonMgr_->GetButtonKeepTime(BUTTON_BOOT) >
           HIBER_BTN_PRESS_TIME)
        {
            Init();
            return;
        }

        delay(100);
    } while(btState == BTN_STATE_DOWN ||
            btState == BTN_STATE_KEEP);

    /* We did not wait for the amount of time */
    Hibernate(false);
}

void SystemState::SetBatteryPercent(const uint8_t kNewBatteryPercent)
{
    batteryPercent_ = kNewBatteryPercent;
    displayNeedUpdate_ = true;
}

uint8_t SystemState::GetBatteryPercent(void) const
{
    return batteryPercent_;
}

void SystemState::SetBrightness(const uint8_t kNewBrightness)
{
    currentBrightness_ = kNewBrightness;
    Storage::GetInstance()->SaveBrightness(kNewBrightness);
    displayNeedUpdate_ = true;
}

uint8_t SystemState::GetBrightness(void) const
{
    return currentBrightness_;
}

const char* SystemState::GetBLEToken(void) const
{
    return pCurrentBLEToken_;
}

void SystemState::SetBLEToken(const char* kpNewToken)
{
    size_t tokenSize;
    tokenSize = strlen(kpNewToken);
    if(tokenSize == BLE_TOCKEN_SIZE)
    {
        memcpy(pCurrentBLEToken_, kpNewToken, BLE_TOCKEN_SIZE);
        Storage::GetInstance()->SaveToken(std::string(kpNewToken));
        displayNeedUpdate_ = true;
    }
    else
    {
        LOG_ERROR("Invalid new token size: %d.\n", tokenSize);
    }
}

const char* SystemState::GetBLEPin(void) const
{
    return pCurrentBLEPIN_;
}

void SystemState::SetBLEPin(const char* kpNewPin)
{
    size_t pinSize;
    pinSize = strlen(kpNewPin);
    if(pinSize <= BLE_PIN_SIZE_MAX)
    {
        memcpy(pCurrentBLEPIN_, kpNewPin, pinSize);
        Storage::GetInstance()->SaveToken(std::string(kpNewPin));

        displayNeedUpdate_ = true;
    }
    else
    {
        LOG_ERROR("Invalid new PIN size: %d.\n", pinSize);
    }
}

SystemState::SystemState(void)
{

}

void SystemState::Init(void)
{
    Storage*    pStorage;
    std::string buffer;

    pStorage = Storage::GetInstance();

    batteryPercent_    = 0;
    lastEventTime_     = 0;
    currentState_      = SYS_MENU_0;
    previousState_     = SYS_IDLE;
    displayNeedUpdate_ = true;

    memset(pButtonsState_,
           EButtonState::BTN_STATE_DOWN,
           sizeof(EButtonState) * EButtonID::BUTTON_MAX_ID);
    memset(pPrevButtonsState_,
           EButtonState::BTN_STATE_DOWN,
           sizeof(EButtonState) * EButtonID::BUTTON_MAX_ID);
    memset(pButtonsKeepTime_,
           0,
           sizeof(uint64_t) * EButtonID::BUTTON_MAX_ID);

    /* Load brightness */
    currentBrightness_ = pStorage->GetBrightness();

    /* Load PIN and token */
    pStorage->GetPin(buffer);
    memcpy(pCurrentBLEPIN_,
           buffer.c_str(),
           MIN(BLE_PIN_SIZE_MAX, buffer.size()));
    pCurrentBLEPIN_[MIN(BLE_PIN_SIZE_MAX, buffer.size())] = 0;

    pStorage->GetToken(buffer);
    memcpy(pCurrentBLEToken_,
           buffer.c_str(),
           MIN(BLE_TOCKEN_SIZE, buffer.size()));
    pCurrentBLEToken_[MIN(BLE_TOCKEN_SIZE, buffer.size())] = 0;

    /* Initialize OLED display */
    oledDisplay_.Init();
}

void SystemState::UpdateState(void)
{
    uint8_t      i;
    EButtonState newState;
    uint64_t     newKeepTime;
    uint64_t     timeNow;
    IOButtonMgr* pButtonMgr_;

    pButtonMgr_ = IOButtonMgr::GetInstance();

    /* Update the buttons state */
    timeNow = HWLayer::GetTime();
    for(i = 0; i < BUTTON_MAX_ID; ++i)
    {
        pPrevButtonsState_[i] = pButtonsState_[i];

        newState = pButtonMgr_->GetButtonState((EButtonID)i);
        if(pButtonsState_[i] != newState)
        {
            pButtonsState_[i] = newState;
            lastEventTime_    = timeNow;
        }
        newKeepTime = pButtonMgr_->GetButtonKeepTime((EButtonID)i);
        if(pButtonsKeepTime_[i] != newKeepTime)
        {
            pButtonsKeepTime_[i] = newKeepTime;
            lastEventTime_       = timeNow;
        }
    }

    /* Check if user is requesting hibernation and manages if we just booted
     * and the user still presses the button.
     */
    if(pButtonsKeepTime_[BUTTON_BOOT] >= HIBER_BTN_PRESS_TIME &&
       pButtonsKeepTime_[BUTTON_BOOT] < HIBER_BTN_PRESS_TIME + 100000)
    {
        Hibernate(true);
    }

    /* Manage IDLE detection in menu */
    if(currentState_ != SYS_IDLE)
    {
        if(HWLayer::GetTime() - lastEventTime_ > SYSTEM_IDLE_TIME)
        {
            SetSystemState(SYS_IDLE);
        }
    }
}

void SystemState::SetSystemState(const ESystemState kNewState)
{
    previousState_     = currentState_;
    currentState_      = kNewState;
    lastEventTime_     = HWLayer::GetTime();
    displayNeedUpdate_ = true;
}

void SystemState::ManageIdle(void)
{
    /* Check if we should restore previous state */
    if(pButtonsState_[BUTTON_ENTER] == BTN_STATE_KEEP &&
       pButtonsKeepTime_[BUTTON_ENTER] >= MENU_BTN_PRESS_TIME)
    {
        LOG_DEBUG("IDLE mode exit\n");

        /* Enable display again */
        oledDisplay_.GetDisplay()->ssd1306_command(SSD1306_DISPLAYON);

        SetSystemState(SYS_MENU_0);
    }
    else if(previousState_ != SYS_IDLE &&
            pButtonsState_[BUTTON_ENTER] == BTN_STATE_UP)
    {
        LOG_DEBUG("IDLE mode enter\n");
        SetSystemState(SYS_IDLE);

        /* Disable display */
        oledDisplay_.SwitchOff();
    }
}

void SystemState::ManageMenu0(void)
{
    uint8_t           battLineSize;
    Adafruit_SSD1306* pOLEDDisplay;
    char              pBleToken[BLE_TOCKEN_SIZE + 1];
    char              pBlePIN[BLE_PIN_SIZE_MAX + 1];

    /* Manage if we should switch to the next menu */
    if(pPrevButtonsState_[BUTTON_ENTER] != BTN_STATE_DOWN &&
       pButtonsState_[BUTTON_ENTER] == BTN_STATE_DOWN)
    {
        SetSystemState(SYS_MENU_1);
        return;
    }

    /* Check if we should update */
    if(displayNeedUpdate_ == true)
    {
        pOLEDDisplay = oledDisplay_.GetDisplay();

        pOLEDDisplay->ssd1306_command(SSD1306_DISPLAYON);
        pOLEDDisplay->clearDisplay();
        pOLEDDisplay->setTextSize(1);
        pOLEDDisplay->setTextColor(WHITE);
        pOLEDDisplay->setCursor(0, 0);
        pOLEDDisplay->fillRect(0, 0, 128, 64, BLACK);

        /* Draw Title */
        pOLEDDisplay->printf("%s\n", HWLayer::GetHWUID());
        pOLEDDisplay->printf("SW %s\n", VERSION_SHORT);

        /* Draw Battery */
        pOLEDDisplay->drawRect(101, 0, 27, 7, WHITE);
        if(batteryPercent_ > 0)
        {
            battLineSize = batteryPercent_ / 4;
            pOLEDDisplay->fillRect(102, 1, battLineSize, 5, WHITE);
        }

        pOLEDDisplay->setCursor(106, 8);
        pOLEDDisplay->printf("%d%%", batteryPercent_);

        /* Draw UI aesthetic*/
        pOLEDDisplay->setCursor(0, 16);

        /* Print PIN */
        memcpy(pBlePIN, pCurrentBLEPIN_, BLE_PIN_SIZE_MAX);
        pBlePIN[BLE_PIN_SIZE_MAX] = 0;
        pOLEDDisplay->printf("PN | %s\n", pBlePIN);
        memcpy(pBleToken, pCurrentBLEToken_, BLE_TOCKEN_SIZE);
        pBleToken[BLE_TOCKEN_SIZE] = 0;
        pOLEDDisplay->printf("TK | %s\n", pBleToken);

        /* Print the current settings */
        pOLEDDisplay->printf("---------------------");
        pOLEDDisplay->printf("Preset     | %4u\n",
                             StripsManager::GetInstance()->GetSelectedScene());
        pOLEDDisplay->printf("Brightness | %3d%%\n", currentBrightness_ * 100 / 255);

        pOLEDDisplay->display();

        displayNeedUpdate_ = false;
    }
}

void SystemState::ManageMenu1(void)
{
    Adafruit_SSD1306* pOLEDDisplay;
    SStorageStats     storageStats;

    /* Manage if we should switch to the next menu */
    if(pPrevButtonsState_[BUTTON_ENTER] != BTN_STATE_DOWN &&
       pButtonsState_[BUTTON_ENTER] == BTN_STATE_DOWN)
    {
        SetSystemState(SYS_MENU_0);
        return;
    }

    /* Check if we should update */
    if(displayNeedUpdate_ == true)
    {
        pOLEDDisplay = oledDisplay_.GetDisplay();

        pOLEDDisplay->ssd1306_command(SSD1306_DISPLAYON);
        pOLEDDisplay->clearDisplay();
        pOLEDDisplay->setTextSize(1);
        pOLEDDisplay->setTextColor(WHITE);
        pOLEDDisplay->fillRect(0, 0, 128, 64, BLACK);
        pOLEDDisplay->setCursor(0, 0);

        /* Infos */
        pOLEDDisplay->printf("InfoV | %s\n", VERSION);
        pOLEDDisplay->printf("CPU: %uMHz\n", ESP.getCpuFreqMHz());
        pOLEDDisplay->printf("Free Heap  | %ukB\n", ESP.getMinFreeHeap() / 1024);
        pOLEDDisplay->printf("Free PSRAM | %ukB\n", ESP.getMinFreePsram() / 1024);

        Storage::GetInstance()->GetStorageStats(storageStats);
        pOLEDDisplay->printf("Storage: %u%% used\n%ukB/%ukB",
                             storageStats.usedSize * 100 / storageStats.totalSize,
                             storageStats.usedSize / 1024,
                             storageStats.totalSize / 1024);

        pOLEDDisplay->display();

        displayNeedUpdate_ = false;
    }
}

void SystemState::Hibernate(const bool kDisplay)
{
    esp_err_t status;
    status = esp_sleep_enable_ext1_wakeup((uint64_t)pow(2ULL, (double)EButtonPin::BOOT_PIN),
                                          ESP_EXT1_WAKEUP_ANY_HIGH);

    if(status == ESP_OK)
    {
        LOG_DEBUG("Enabling Deep Sleep\n");

        if(kDisplay == true)
        {
            oledDisplay_.DisplaySleep();
            StripsManager::GetInstance()->Kill();
            delay(3000);
            oledDisplay_.SwitchOff();
        }
        else
        {
            StripsManager::GetInstance()->Kill();
            delay(500);
        }

        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
        esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);

        esp_deep_sleep_start();
    }
    else
    {
        LOG_ERROR("Could not setup deep sleep wakeup (%d)\n", status);
    }
}