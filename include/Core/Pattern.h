/*******************************************************************************
 * @file Pattern.h
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

#ifndef __CORE_PATTERN_H_
#define __CORE_PATTERN_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cstdint> /* Standard Int Types */
#include <vector>  /* CPP vectors */
#include <Types.h> /* Defined types */


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

typedef enum
{
    ANIM_TRAIL,
    ANIM_BREATH
} EAnimationType;

typedef struct
{
    uint16_t startIdx;
    uint16_t endIdx;

    uint32_t startColorCode;
    uint32_t endColorCode;
} SColor;

typedef struct
{
    EAnimationType type;
    uint16_t       startIdx;
    uint16_t       endIdx;
    uint8_t        param;
} SAnimation;

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

class Pattern
{
    /********************* PUBLIC METHODS AND ATTRIBUTES **********************/
    public:
        Pattern(const uint16_t kIdentifier,
                const std::string& krName);

        void SetAnimations(const std::vector<SAnimation>& rAnimations);
        void SetColors(const std::vector<SColor>& rColors);
        void SetBrightness(const uint8_t kBrightness);

        const std::vector<SAnimation>& GetAnimations(void) const;
        const std::vector<SColor>& GetColors(void) const;
        uint8_t GetBrightness(void) const;

        uint16_t GetId(void) const;
        const std::string& GetName(void) const;

        void SetLastChangeTime(const uint64_t kLastChangeTime);

    /******************* PROTECTED METHODS AND ATTRIBUTES *********************/
    protected:

    /********************* PRIVATE METHODS AND ATTRIBUTES *********************/
    private:
        uint16_t    identifier_;
        std::string name_;

        uint8_t                 brightness_;
        std::vector<SAnimation> animations_;
        std::vector<SColor>     colors_;
};

#endif /* #ifndef __CORE_PATTERN_H_ */