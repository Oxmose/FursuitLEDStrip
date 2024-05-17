/*******************************************************************************
 * @file LEDStrip.hpp
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/04/2024
 *
 * @version 1.0
 *
 * @brief This file defines the LED strip driver.
 *
 * @details This file defines the LED strip driver used in the ESP32 module.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __BSP_LEDSTRIP_H_
#define __BSP_LEDSTRIP_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cstdint> /* Standard Int Types */
#include <memory>  /* std::shared_ptr */
#include <vector>  /* std::vector */
#include <string> /* std::string */
#include <Arduino.h>  /* Arduino Services */
#include <FastLED.h> /* FastLED driver */
#include <Logger.h>  /* Logger service */
#include <Pattern.h> /* Pattern Object */

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

typedef struct SStripInfo
{
    gpio_num_t  ctrlGPIO;
    uint16_t    numLed;
    bool        isEnabled;
    std::string name;
} SStripInfo;

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

class LEDStrip
{
    public:
        virtual ~LEDStrip(void) {};

        virtual void GetStripInfo(std::shared_ptr<SStripInfo> info) const = 0;
        virtual uint8_t GetId(void) const = 0;
        virtual const std::string& GetName(void) const = 0;

        virtual void Apply(const Pattern* pPattern) = 0;
        virtual void UpdateColors(void) = 0;

        virtual void SetEnabled(const bool kEnable) = 0;
        virtual bool IsEnabled(void) const = 0;
};

template<gpio_num_t kGpioId, gpio_num_t kMosfetId, uint16_t kNumLeds>
class LEDStripC : public LEDStrip
{
    public:
        LEDStripC(const char* kpName) :
        rCtrl_(FastLED.addLeds<WS2812B, (uint8_t)kGpioId, GRB>(leds_,
                                                               (int)kNumLeds))
        {
            name_         = std::string(kpName);
            isEnabled_    = true;
            applyIter_    = 0;
            breathIn_     = false;
            updateColors_ = true;

            SetEnabled(false);

            rCtrl_.setCorrection(TypicalLEDStrip);
        }

        virtual ~LEDStripC(void)
        {
        }

        virtual void GetStripInfo(std::shared_ptr<SStripInfo> info) const
        {
            info->ctrlGPIO  = kGpioId;
            info->numLed    = kNumLeds;
            info->name      = name_;
            info->isEnabled = isEnabled_;
        }

        virtual uint8_t GetId(void) const
        {
            return (uint8_t)kGpioId;
        }

        virtual const std::string& GetName(void) const
        {
            return name_;
        }

        virtual void Apply(const Pattern* pPattern)
        {
            if(isEnabled_ == false)
            {
                return;
            }

            /* Apply colors */
            if(updateColors_ == true)
            {
                ApplyColor(pPattern->GetColors(),
                           pPattern->GetBrightness());

                updateColors_ = false;
            }

            /* Apply animations */
            const std::vector<SAnimation>& krAnims = pPattern->GetAnimations();
            for(const SAnimation& krAnim : krAnims)
            {
                switch(krAnim.type)
                {
                    case ANIM_TRAIL:
                        ApplyTrail(krAnim);
                        break;
                    case ANIM_BREATH:
                        ApplyBreath(krAnim);
                        break;
                    default:
                        LOG_ERROR("Unknown animation ID %d\n", krAnim.type);
                        break;
                }
            }

            ++applyIter_;
        }

        virtual void UpdateColors(void)
        {
            updateColors_ = true;
        }

        virtual void SetEnabled(const bool kEnable)
        {
            if(kEnable == false && isEnabled_ == true)
            {
                /* Switch off the mosfet */
                pinMode(kMosfetId, OUTPUT);
                digitalWrite(kMosfetId, LOW);

                /* Stop controller pin on the mosfet */
                pinMode(kGpioId, OUTPUT);
                digitalWrite(kGpioId, LOW);

                LOG_DEBUG("Disabling Strip %d\n", kGpioId);
            }
            else if(kEnable == true && isEnabled_ == false)
            {
                /* Switch on the mosfet */
                pinMode(kMosfetId, OUTPUT);
                digitalWrite(kMosfetId, HIGH);

                LOG_DEBUG("Enabling Strip %d\n", kGpioId);
            }
            isEnabled_ = kEnable;
        }

        virtual bool IsEnabled(void) const
        {
            return isEnabled_;
        }

    protected:

    private:
        void ApplyColor(const std::vector<SColor>& krColors,
                        const uint8_t kBrightness)
        {
            uint32_t i;

            rCtrl_.clearLedData();

            for(const SColor& krColor : krColors)
            {
                /* Check if gradient */
                if(krColor.startColorCode != krColor.endColorCode)
                {
                    /* Create the gradient */
                    fill_gradient_RGB(&leds_[krColor.startIdx],
                                    krColor.endIdx - krColor.startIdx + 1,
                                    krColor.startColorCode,
                                    krColor.endColorCode);
                }
                else
                {
                    /* Just set the color */
                    for(i = krColor.startIdx; i <= krColor.endIdx; ++i)
                    {
                        leds_[i] = krColor.startColorCode;
                    }
                }
            }

            /* Apply brightness */
            brightness_    = kBrightness;
            maxBrightness_ = kBrightness;
            for(i = 0; i < kNumLeds; ++i)
            {
                ledsInit_[i] = leds_[i];
                leds_[i].g = (uint8_t)((uint32_t)leds_[i].g * (uint32_t)brightness_ / 255U);
                leds_[i].r = (uint8_t)((uint32_t)leds_[i].r * (uint32_t)brightness_ / 255U);
                leds_[i].b = (uint8_t)((uint32_t)leds_[i].b * (uint32_t)brightness_ / 255U);
            }
        }

        void ApplyTrail(const SAnimation& krAnim)
        {
            uint32_t i;
            CRGB     swap;

            /* Param is for the speed */
            if(applyIter_ % krAnim.param == 0)
            {
                /* Check for reverse */
                if(krAnim.startIdx > krAnim.endIdx)
                {
                    swap = leds_[krAnim.startIdx];
                    for(i = krAnim.startIdx; i > krAnim.endIdx; --i)
                    {
                        leds_[i] = leds_[i - 1];
                    }
                    leds_[krAnim.endIdx] = swap;
                }
                else
                {
                    swap = leds_[krAnim.startIdx];
                    for(i = krAnim.startIdx; i < krAnim.endIdx; ++i)
                    {
                        leds_[i] = leds_[i + 1];
                    }
                    leds_[krAnim.endIdx] = swap;
                }
            }
        }

        void ApplyBreath(const SAnimation& krAnim)
        {
            uint32_t i;

            /* Param is for the speed */
            if(applyIter_ % krAnim.param == 0)
            {
                if(breathIn_ == true)
                {
                    if(brightness_ + 1 <= maxBrightness_)
                    {
                        ++brightness_;
                    }
                    else
                    {
                        brightness_ = maxBrightness_;
                        breathIn_ = false;
                    }
                }
                else
                {
                    if(brightness_ >= 1)
                    {
                        --brightness_;
                    }
                    else
                    {
                        breathIn_ = true;
                    }
                }

                for(i = krAnim.startIdx; i <= krAnim.endIdx; ++i)
                {
                    leds_[i].g = (uint8_t)((uint32_t)ledsInit_[i].g * (uint32_t)brightness_ / 255U);
                    leds_[i].r = (uint8_t)((uint32_t)ledsInit_[i].r * (uint32_t)brightness_ / 255U);
                    leds_[i].b = (uint8_t)((uint32_t)ledsInit_[i].b * (uint32_t)brightness_ / 255U);
                }
            }
        }

        bool     isEnabled_;
        bool     updateColors_;
        bool     breathIn_;
        uint32_t applyIter_;
        uint8_t  brightness_;
        uint8_t  maxBrightness_;

        std::string name_;

        CLEDController& rCtrl_;
        CRGB            leds_[kNumLeds];
        CRGB            ledsInit_[kNumLeds];
};


#endif /* #ifndef __BSP_LEDSTRIP_H_ */