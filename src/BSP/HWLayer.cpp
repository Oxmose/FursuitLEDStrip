/*******************************************************************************
 * @file HWLayer.cpp
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 20/04/2024
 *
 * @version 1.0
 *
 * @brief This file defines the hardware layer.
 *
 * @details This file defines the hardware layer. This layer provides services
 * to interact with the ESP32 module hardware.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/


/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <string>  /* std::string*/
#include <WiFi.h>  /* Mac address provider */
#include <Types.h> /* Defined Types */

/* Header File */
#include <HWLayer.h>

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
std::string HWLayer::HWUID_;
std::string HWLayer::MACADDR_;
uint64_t    HWLayer::TIME_ = 0;

/************************** Static global variables ***************************/
static const char spkHexTable[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

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

const char * HWLayer::GetHWUID(void)
{
    uint32_t uid;
    uint8_t  i;
    uint8_t  curVal;

    /* Check if the HWUID was already generated */
    if(HWLayer::HWUID_.size() == 0)
    {
        HWLayer::HWUID_ = "FSL-";

        uid = (uint32_t)ESP.getEfuseMac();

        for(i = 0; i < 8; ++i)
        {
            curVal = uid >> (28 - i * 4);
            HWLayer::HWUID_ += spkHexTable[curVal & 0xF];
        }
    }

    /* Copy HWUID */
    return HWLayer::HWUID_.c_str();
}

const char * HWLayer::GetMacAddress(void)
{
    uint8_t  i;
    uint8_t  value[6];

    /* Check if the HWUID was already generated */
    if(HWLayer::MACADDR_.size() == 0)
    {
        esp_read_mac(value, ESP_MAC_BT);

        HWLayer::MACADDR_ = "";
        for(i = 0; i < 6; ++i)
        {
            HWLayer::MACADDR_ += std::string(1, spkHexTable[(value[i] >> 4) & 0xF]) +
                                 std::string(1, spkHexTable[value[i] & 0xF]);
            if(i < 5)
            {
                HWLayer::MACADDR_ += ":";
            }
        }
    }

    /* Copy HWUID */
    return HWLayer::MACADDR_.c_str();
}

uint64_t HWLayer::GetTime(void)
{
    HWLayer::TIME_ = (uint64_t)esp_timer_get_time();

    return HWLayer::TIME_;
}