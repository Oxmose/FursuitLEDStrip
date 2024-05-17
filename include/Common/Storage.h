/*******************************************************************************
 * @file Storage.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 02/05/2024
 *
 * @version 1.0
 *
 * @brief Storage abstraction layer.
 *
 * @details This file provides the storage manager service.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __COMMON_STORAGE_H_
#define __COMMON_STORAGE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cstdint> /* Standard Int Types */
#include <vector>  /* std::vector */
#include <memory>  /* std::shared_ptr */
#include <utility> /* std::pair */
#include <unordered_map> /* std::unordered_map */
#include <Pattern.h> /* Patern object */
#include <StripsManager.h> /* Strip manager types */

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

typedef std::pair<std::vector<std::shared_ptr<SScene>>, bool>  ScenesCache;
typedef std::pair<std::vector<std::shared_ptr<Pattern>>, bool> PatternCache;
typedef std::pair<std::string, bool>                           StringCache;
typedef std::pair<uint8_t, bool>                               Uint8Cache;

typedef struct
{
    uint32_t totalSize;
    uint32_t usedSize;
} SStorageStats;

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

class Storage
{
    /********************* PUBLIC METHODS AND ATTRIBUTES **********************/
    public:
        static Storage* GetInstance(void);

        void LoadData(void);

        void Update(const bool kForce);

        void GetPatterns(std::vector<std::shared_ptr<Pattern>>& rPatterns) const;
        void SavePatterns(const std::vector<std::shared_ptr<Pattern>>& krPatterns);

        void GetScenes(std::vector<std::shared_ptr<SScene>>& rScenes) const;
        void SaveScenes(const std::vector<std::shared_ptr<SScene>>& krScenes);

        void SaveSelectedScene(const uint8_t kSelectedScene);

        uint8_t GetBrightness(void) const;
        void SaveBrightness(const uint8_t kBrightness);

        void GetToken(std::string& rStrToken) const;
        void SaveToken(const std::string& krStrToken);

        void GetPin(std::string& rStrPin) const;
        void SavePin(const std::string& krStrPin);

        void GetStorageStats(SStorageStats& rState);

    /******************* PROTECTED METHODS AND ATTRIBUTES *********************/
    protected:

    /********************* PRIVATE METHODS AND ATTRIBUTES *********************/
    private:
        Storage(void);

        void Commit(const bool kForce);

        void WriteFile(const char* kpPath,
                       const uint8_t* kpBuffer,
                       size_t& rSize) const;
        void ReadFile(const char* kpPath,
                      uint8_t* pBuffer,
                      size_t& rSize) const;

        void LoadPatterns(void);
        void CommitPattern(const std::shared_ptr<Pattern>& krPattern) const;

        void LoadScenes(void);
        void CommitScenes(void) const;

        void FactoryReset(void);

        bool     isInit_;
        bool     needUpdate_;
        uint64_t lastUpdateTime_;

        /* Cached data */
        ScenesCache  scenes_;
        PatternCache patterns_;
        StringCache  pin_;
        StringCache  token_;
        Uint8Cache   brightness_;
        Uint8Cache   selectedScene_;

        /* Instance */
        static Storage* PINSTANCE_;
};

#endif /* #ifndef __COMMON_STORAGE_H_ */