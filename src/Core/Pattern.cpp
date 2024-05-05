/*******************************************************************************
 * @file Pattern.cpp
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 01/05/2024
 *
 * @version 1.0
 *
 * @brief LED strip pattern.
 *
 * @details This file provides the LED strip pattern class.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <cstdint>    /* Standard Int Types */
#include <Types.h>    /* Defined types */
#include <Logger.h> /* Logger services */
#include <string>  /* std::string */

/* Header File */
#include <Pattern.h>

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
 * CLASS METHODS
 ******************************************************************************/

Pattern::Pattern(const uint16_t kIdentifier, const std::string& krName)
{
    identifier_ = kIdentifier;
    name_       = krName;
}

void Pattern::SetAnimations(const std::vector<SAnimation>& rAnimations)
{
    animations_ = rAnimations;
}

void Pattern::SetColors(const std::vector<SColor>& rColors)
{
    colors_ = rColors;
}

void Pattern::SetBrightness(const uint8_t kBrightness)
{
    brightness_ = kBrightness;
}

const std::vector<SAnimation>& Pattern::GetAnimations(void) const
{
    return animations_;
}

const std::vector<SColor>& Pattern::GetColors(void) const
{
    return colors_;
}

uint8_t Pattern::GetBrightness(void) const
{
    return brightness_;
}

uint16_t Pattern::GetId(void) const
{
    return identifier_;
}

const std::string& Pattern::GetName(void) const
{
    return name_;
}