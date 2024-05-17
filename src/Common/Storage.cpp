/*******************************************************************************
 * @file Storage.cpp
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cstdint> /* Standard Int Types */
#include <vector>  /* std::vector */
#include <memory>  /* std::shared_ptr */
#include <utility> /* std::pair */
#include <unordered_map> /* std::unordered_map */
#include <FS.h>    /* Filesystem services */
#include <SPIFFS.h> /* SPIFFS driver */
#include <sys/stat.h> /* stat services */
#include <Pattern.h> /* Patern object */
#include <HWLayer.h> /* Hardware layer services */
#include <Logger.h> /* Logger service */

/* Header file */
#include <Storage.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define COMMIT_TIME_SYNC  120000000UL // 120s in us
#define BUFFER_SIZE       512
#define BIG_BUFFER_SIZE   16384

#define INIT_FILE_PATH      "/init"
#define BLE_PIN_PATH        "/pin"
#define BLE_TOKEN_PATH      "/token"
#define BRIGHTNESS_PATH     "/brightness"
#define PATTERN_PATH        "/pattern_"
#define SCENES_PATH         "/scenes"
#define SELECTED_SCENE_PATH "/selected_scene"

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
Storage* Storage::PINSTANCE_ = nullptr;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

Storage* Storage::GetInstance(void)
{
    if (Storage::PINSTANCE_ == nullptr)
    {
        Storage::PINSTANCE_ = new Storage();
    }

    return Storage::PINSTANCE_;
}

void Storage::Update(const bool kForce)
{
    uint64_t currTime;

    if(isInit_ == false)
    {
        return;
    }

    /* Check if we need to write the flash  */
    currTime = HWLayer::GetTime();
    if((needUpdate_ && (currTime - lastUpdateTime_ > COMMIT_TIME_SYNC)) ||
        kForce)
    {
        /* Commit data to the flash */
        Commit(kForce);

        /* Update state and last update time */
        lastUpdateTime_ = currTime;
        needUpdate_     = false;
    }
}

void Storage::GetPatterns(std::vector<std::shared_ptr<Pattern>>& rPatterns) const
{
    if(isInit_ == false)
    {
        return;
    }

    rPatterns = patterns_.first;
}

void Storage::SavePatterns(const std::vector<std::shared_ptr<Pattern>>& krPatterns)
{
    if(isInit_ == false)
    {
        return;
    }

    patterns_.first  = krPatterns;
    patterns_.second = true;

    needUpdate_ = true;
}

void Storage::GetScenes(std::vector<std::shared_ptr<SScene>>& rScenes) const
{
    if(isInit_ == false)
    {
        return;
    }

    rScenes = scenes_.first;
}

void Storage::SaveScenes(const std::vector<std::shared_ptr<SScene>>& krScenes)
{
    if(isInit_ == false)
    {
        return;
    }

    scenes_.first  = krScenes;
    scenes_.second = true;

    needUpdate_ = true;
}

void Storage::SaveSelectedScene(const uint8_t kSelectedScene)
{
    if(isInit_ == false)
    {
        return;
    }

    selectedScene_.first  = kSelectedScene;
    selectedScene_.second = true;

    needUpdate_ = true;
}

uint8_t Storage::GetBrightness(void) const
{
    if(isInit_ == false)
    {
        return 0;
    }

    return brightness_.first;
}

void Storage::SaveBrightness(const uint8_t kBrightness)
{
    if(isInit_ == false)
    {
        return;
    }

    brightness_.first  = kBrightness;
    brightness_.second = true;

    needUpdate_ = true;
}

void Storage::GetToken(std::string& rStrToken) const
{
    if(isInit_ == false)
    {
        return;
    }

    rStrToken = token_.first;
}

void Storage::SaveToken(const std::string& krStrToken)
{
    if(isInit_ == false)
    {
        return;
    }

    token_.first  = krStrToken;
    token_.second = true;

    needUpdate_ = true;
}

void Storage::GetPin(std::string& rStrPin) const
{
    if(isInit_ == false)
    {
        return;
    }

    rStrPin = pin_.first;
}

void Storage::SavePin(const std::string& krStrPin)
{
    if(isInit_ == false)
    {
        return;
    }

    pin_.first  = krStrPin;
    pin_.second = true;

    needUpdate_ = true;
}

void Storage::GetStorageStats(SStorageStats& rStats)
{
    if(isInit_ == false)
    {
        rStats.totalSize = 0;
        rStats.usedSize  = 0;
        return;
    }

    rStats.totalSize = SPIFFS.totalBytes();
    rStats.usedSize  = SPIFFS.usedBytes();
}

Storage::Storage(void)
{
    /* Init the SPIFFS */
    if(SPIFFS.begin(true) == false)
    {
        LOG_ERROR("Failed to mount the SPIFFS\n");
        isInit_ = false;
        return;
    }

    /* Check if we has a populated storage */
    if(SPIFFS.exists(INIT_FILE_PATH) == false)
    {
        /* If not populated, perform first reset */
        FactoryReset();
    }
    else
    {
        LOG_DEBUG("Flash already initialized.\n");
    }

    isInit_ = true;
}

void Storage::LoadData(void)
{
    uint8_t* pBuffer;
    size_t   readSize;

    if(isInit_ == false)
    {
        LOG_ERROR("Cannot load data, not initialized\n");
        return;
    }

    needUpdate_     = false;
    lastUpdateTime_ = 0;

    pBuffer = new uint8_t[BUFFER_SIZE];

    /* Load Pin */
    readSize = BUFFER_SIZE;
    ReadFile(BLE_PIN_PATH, pBuffer, readSize);
    if(readSize != 0)
    {
        pBuffer[readSize] = 0;
        pin_.first = (char*)pBuffer;
    }
    else
    {
        pin_.first = "0000";
        LOG_ERROR("Could not load pin\n");
    }
    pin_.second = false;

    /* Load Token */
    readSize = BUFFER_SIZE;
    memset(pBuffer, 0, BUFFER_SIZE);
    ReadFile(BLE_TOKEN_PATH, pBuffer, readSize);
    if(readSize != 0)
    {
        pBuffer[readSize] = 0;
        token_.first = (char*)pBuffer;
    }
    else
    {
        token_.first = "0000";
        LOG_ERROR("Could not load token\n");
    }
    token_.second = false;

    /* Load Brightness */
    readSize = BUFFER_SIZE;
    ReadFile(BRIGHTNESS_PATH, pBuffer, readSize);
    if(readSize == sizeof(uint8_t))
    {
        brightness_.first = *pBuffer;
    }
    else
    {
        brightness_.first = 0;
        LOG_ERROR("Could not load brightness\n");
    }
    brightness_.second = false;

    delete[] pBuffer;

    /* Load Patterns */
    LoadPatterns();
    patterns_.second = false;

    /* Load links */
    LoadScenes();
    scenes_.second = false;

    LOG_INFO("Storage Initialized.\n");
}

void Storage::Commit(const bool kForce)
{
    size_t size;

    if(needUpdate_ == true || kForce == true)
    {
        if(pin_.second == true)
        {
            size = pin_.first.size();
            WriteFile(BLE_PIN_PATH, (uint8_t*)pin_.first.c_str(), size);

            pin_.second = false;
        }

        if(token_.second == true)
        {
            size = token_.first.size();
            WriteFile(BLE_TOKEN_PATH, (uint8_t*)token_.first.c_str(), size);

            token_.second = false;
        }

        if(brightness_.second == true)
        {
            size = sizeof(brightness_.first);
            WriteFile(BRIGHTNESS_PATH, &brightness_.first, size);

            brightness_.second = false;
        }

        if(patterns_.second == true)
        {
            for(const std::shared_ptr<Pattern>& krPattern : patterns_.first)
            {
                CommitPattern(krPattern);
            }

            patterns_.second = false;
        }

        if(scenes_.second == true)
        {
            CommitScenes();

            scenes_.second = false;
        }

        if(selectedScene_.second == true)
        {
            size = sizeof(selectedScene_.first);
            WriteFile(SELECTED_SCENE_PATH, &selectedScene_.first, size);

            selectedScene_.second = false;
        }

        LOG_DEBUG("Commited cache\n");
    }
}

void Storage::WriteFile(const char* kpPath,
                        const uint8_t* kpBuffer,
                        size_t& rSize) const
{
    File file;

    /* Remove if exists */
    if(SPIFFS.exists(kpPath))
    {
        LOG_DEBUG("Removing %s\n", kpPath);
        SPIFFS.remove(kpPath);
    }

    LOG_DEBUG("Creating %s\n", kpPath);
    file = SPIFFS.open(kpPath, FILE_WRITE);
    if(!file)
    {
        LOG_ERROR("Failed to open %s for writting\n", kpPath);
        rSize = 0;
        return;
    }

    /* Write to file */
    LOG_DEBUG("Writing %s\n", kpPath);
    if((rSize = file.write(kpBuffer, rSize)) == 0)
    {
        LOG_ERROR("Could not wirte file %s\n", file.name());
    }
    else
    {
        LOG_INFO("Wrote file %s\n", file.name());
    }

    file.close();
}

void Storage::ReadFile(const char* kpPath,
                       uint8_t* pBuffer,
                       size_t& rSize) const
{
    File    file;
    ssize_t readBytes;
    ssize_t leftToRead;
    ssize_t offset;

    if(isInit_ == false)
    {
        return;
    }

    /* Open file */
    file = SPIFFS.open(kpPath);
    if(!file || file.isDirectory())
    {
        LOG_ERROR("Failed to open %s\n", kpPath)
        rSize = 0;
        return;
    }

    /* Read file */
    leftToRead = rSize;
    offset     = 0;
    while(file.available() && leftToRead > 0)
    {
        readBytes = file.read(pBuffer + offset, leftToRead);
        if(readBytes > 0)
        {
            offset     += readBytes;
            leftToRead -= readBytes;
        }
        else
        {
            break;
        }
    }
    rSize -= leftToRead;
    file.close();
    LOG_DEBUG("Read %d bytes in %s\n", rSize, kpPath);
}

void Storage::LoadPatterns(void)
{
    File                     root;
    File                     file;
    uint8_t*                 pBuffer;
    const char*              kpPatternName = PATTERN_PATH;
    size_t                   i;
    size_t                   bufferOff;
    size_t                   readSize;
    size_t                   patternPathSize;
    size_t                   readBytes;
    size_t                   elemCount;
    SColor                   loadedColor;
    SAnimation               loadedAnim;
    std::shared_ptr<Pattern> patternPtr;
    std::vector<SColor>      colors;
    std::vector<SAnimation>  anims;

    root = SPIFFS.open("/");
    if(!root)
    {
        LOG_ERROR("Failed to open root\n");
        return;
    }

    ++kpPatternName;
    patternPathSize = strlen(kpPatternName);

    pBuffer = new uint8_t[BUFFER_SIZE];

    file = root.openNextFile();
    while(file)
    {
        if(strncmp(kpPatternName, file.name(), patternPathSize) == 0)
        {
            LOG_DEBUG("Reading %s\n", file.name());

            bufferOff = 0;
            readSize  = BUFFER_SIZE - 1;

            while(file.available() && readSize > 0)
            {
                readBytes = file.read(pBuffer + bufferOff, readSize);
                if(readBytes > 0)
                {
                    bufferOff += readBytes;
                    readSize  -= readBytes;
                }
                else
                {
                    break;
                }
            }

            if(bufferOff != 0)
            {
                bufferOff = 0;

                /* Read identifier and name and create pattern  */
                elemCount = *((uint8_t*)&pBuffer[bufferOff]);
                bufferOff += sizeof(uint8_t);
                i = *((uint16_t*)&pBuffer[bufferOff + elemCount]);
                patternPtr = std::make_shared<Pattern>(i, std::string(&pBuffer[bufferOff],
                                                                      &pBuffer[bufferOff] + elemCount));
                bufferOff += elemCount + sizeof(uint16_t);

                /* Read brightness */
                patternPtr->SetBrightness(*((uint8_t*)&pBuffer[bufferOff]));
                bufferOff += sizeof(uint8_t);

                /* Load animations */
                elemCount = *((uint8_t*)&pBuffer[bufferOff]);
                bufferOff += sizeof(uint8_t);
                LOG_DEBUG("Loading %d animations\n", elemCount);
                anims.resize(elemCount);
                for(i = 0; i < elemCount; ++i)
                {
                    if(bufferOff + sizeof(SAnimation) > BUFFER_SIZE)
                    {
                        LOG_ERROR("Could not load animation, buffer is full %s\n",
                                  bufferOff);
                        delete[] pBuffer;
                        return;
                    }

                    memcpy(&loadedAnim,
                           &pBuffer[bufferOff],
                           sizeof(SAnimation));
                    bufferOff += sizeof(SAnimation);

                    anims[i] = loadedAnim;
                }

                /* Load colors */
                elemCount = *((uint8_t*)&pBuffer[bufferOff]);
                bufferOff += sizeof(uint8_t);
                LOG_DEBUG("Loading %d colors\n", elemCount);
                colors.resize(elemCount);
                for(i = 0; i < elemCount; ++i)
                {
                    if(bufferOff + sizeof(SColor) > BUFFER_SIZE)
                    {
                        LOG_ERROR("Could not load color, buffer is full\n");
                        delete[] pBuffer;
                        return;
                    }

                    memcpy(&loadedColor,
                           &pBuffer[bufferOff],
                           sizeof(SColor));
                    bufferOff += sizeof(SColor);

                    colors[i] = loadedColor;
                }

                /* Add colors and animation to pattern */
                patternPtr->SetAnimations(anims);
                patternPtr->SetColors(colors);

                /* Add the pattern */
                patterns_.first.push_back(patternPtr);

                LOG_DEBUG("Loaded %s\n", file.name());
            }
            else
            {
                LOG_ERROR("Could not load pattern, buffer too small\n");
            }
        }
        else
        {
            LOG_DEBUG("Skipped %s\n", file.name());
        }
        file = root.openNextFile();
    }

    delete[] pBuffer;
}

void Storage::CommitPattern(const std::shared_ptr<Pattern>& krPattern) const
{
    File     file;
    uint8_t* pBuffer;
    size_t   buffSize;
    size_t   nameSize;

    pBuffer = new uint8_t[BUFFER_SIZE];

    /* Create the file or remove it before */
    snprintf((char*)pBuffer,
             BUFFER_SIZE,
             "%s%s",
             PATTERN_PATH,
             std::to_string(krPattern->GetId()).c_str());

    /* Remove if exists */
    if(SPIFFS.exists((char*)pBuffer))
    {
        SPIFFS.remove((char*)pBuffer);
    }

    file = SPIFFS.open((char*)pBuffer, FILE_WRITE);
    if(!file)
    {
        LOG_ERROR("Failed to open %s for writting\n", pBuffer);
        delete[] pBuffer;
        return;
    }

    buffSize = 0;

    /* Save name */
    nameSize = (uint8_t)krPattern->GetName().size();
    if(nameSize > 256)
    {
        LOG_ERROR("Pattern name size too big, truncating to 256\n");
        nameSize = 256;
    }
    *((uint8_t*)&pBuffer[buffSize]) = nameSize;
    buffSize += sizeof(uint8_t);
    memcpy(&pBuffer[buffSize], krPattern->GetName().c_str(), nameSize);
    buffSize += nameSize;

    /* Save identifier */
    *((uint16_t*)&pBuffer[buffSize]) = krPattern->GetId();
    buffSize += sizeof(uint16_t);

    /* Save brightness */
    *((uint8_t*)&pBuffer[buffSize]) = krPattern->GetBrightness();
    buffSize += sizeof(uint8_t);

    /* Save animations */
    const std::vector<SAnimation>& krAnims = krPattern->GetAnimations();
    *((uint8_t*)&pBuffer[buffSize]) = (uint8_t)krAnims.size();
    buffSize += sizeof(uint8_t);
    for(const SAnimation& krAnim : krAnims)
    {
        if(buffSize + sizeof(SAnimation) > BUFFER_SIZE)
        {
            LOG_ERROR("Could not save animation, buffer is full %d\n",
                      buffSize);
            delete[] pBuffer;
            return;
        }

        memcpy(&pBuffer[buffSize], &krAnim, sizeof(SAnimation));
        buffSize += sizeof(SAnimation);
    }

    /* Save colors */
    const std::vector<SColor>& krColors = krPattern->GetColors();
    *((uint8_t*)&pBuffer[buffSize]) = (uint8_t)krColors.size();
    buffSize += sizeof(uint8_t);
    for(const SColor& krColor : krColors)
    {
        if(buffSize + sizeof(SColor) > BUFFER_SIZE)
        {
            LOG_ERROR("Could not save color, buffer is full\n");
            delete[] pBuffer;
            return;
        }

        memcpy(&pBuffer[buffSize], &krColor, sizeof(SColor));
        buffSize += sizeof(SColor);
    }

    /* Write to file */
    if(file.write(pBuffer, buffSize) == 0)
    {
        LOG_ERROR("Could not write file %s\n", file.name());
    }
    else
    {
        LOG_INFO("Saved file %s\n", file.name());
    }

    file.close();

    delete[] pBuffer;
}

void Storage::LoadScenes(void)
{
    size_t   bufferSize;
    size_t   bufferOff;
    size_t   sizeProp;
    uint8_t  scenesCount;
    uint8_t  idx;
    uint8_t  i;
    uint8_t* pBuffer;
    std::shared_ptr<SScene> newScenePtr;

    bufferSize = BIG_BUFFER_SIZE;

    pBuffer = new uint8_t[bufferSize];

    ReadFile(SCENES_PATH, pBuffer, bufferSize);

    if(bufferSize == 0 || bufferSize == BIG_BUFFER_SIZE)
    {
        LOG_ERROR("Could not load links, buffer full or empty\n");
        delete[] pBuffer;
        return;
    }

    bufferOff = 0;

    /* Read number of scenes */
    scenesCount = *(uint8_t*)&pBuffer[bufferOff++];

    /* Get all scenes */
    for(i = 0; i < scenesCount; ++i)
    {
        /* Create scene */
        newScenePtr = std::make_shared<SScene>();

        /* Get name size */
        sizeProp = (size_t)(*(uint8_t*)&pBuffer[bufferOff++]);
        if(sizeProp > 256)
        {
            LOG_ERROR("Cannot load scene, name too big\n");
            continue;
        }

        /* Get name */
        newScenePtr->name = std::string(&pBuffer[bufferOff],
                                        &pBuffer[bufferOff + sizeProp]);
        bufferOff += sizeProp;

        /* Get the number of links */
        sizeProp = (size_t)(*(uint8_t*)&pBuffer[bufferOff++]);
        if(bufferOff + (sizeof(uint8_t) + sizeof(uint16_t)) * sizeProp >
           bufferSize)
        {
            LOG_ERROR("Failed to load scenes, buffer is too small (%d, %d, %d\n", bufferSize, sizeProp, bufferOff);
            delete[] pBuffer;
            return;
        }

        while(sizeProp != 0)
        {
            idx = *(uint8_t*)&pBuffer[bufferOff++];
            newScenePtr->links[idx] = *(uint16_t*)&pBuffer[bufferOff];
            bufferOff += sizeof(uint16_t);
            --sizeProp;
        }

        /* Add scene */
        scenes_.first.push_back(newScenePtr);
    }

    /* Load and cache selected scene */
    bufferSize = BIG_BUFFER_SIZE;
    ReadFile(SELECTED_SCENE_PATH, pBuffer, bufferSize);
    selectedScene_.first  = *(uint8_t*)&pBuffer[0];

    delete[] pBuffer;
}

void Storage::CommitScenes(void) const
{
    size_t   bufferOff;
    size_t   sizeProp;
    uint8_t  scenesCount;
    uint8_t  idx;
    uint8_t  i;
    uint8_t* pBuffer;
    std::shared_ptr<SScene> newScenePtr;
    const std::vector<std::shared_ptr<SScene>>& krScenes = scenes_.first;

    pBuffer = new uint8_t[BIG_BUFFER_SIZE];
    bufferOff = 0;

    /* Save number of scenes */
    scenesCount = (uint8_t)krScenes.size();
    *(uint8_t*)&pBuffer[bufferOff++] = scenesCount;

    /* Save all scenes */
    for(i = 0; i < scenesCount; ++i)
    {
        /* Save name size and name */
        sizeProp = krScenes[i]->name.size();
        if(sizeProp > 256)
        {
            sizeProp = 256;
            LOG_ERROR("Cutting scene name, name too big\n");
        }
        *(uint8_t*)&pBuffer[bufferOff++] = sizeProp;
        memcpy(&pBuffer[bufferOff], krScenes[i]->name.c_str(), sizeProp);
        bufferOff += sizeProp;

        /* Get the number of links */
        sizeProp = krScenes[i]->links.size();
        if(bufferOff + (sizeof(uint8_t) + sizeof(uint16_t)) * sizeProp >
           BIG_BUFFER_SIZE)
        {
            LOG_ERROR("Failed to save scenes, buffer is too small\n");
            delete[] pBuffer;
            return;
        }
        *(uint8_t*)&pBuffer[bufferOff++] = sizeProp;

        for(const std::pair<uint8_t, uint16_t>& krLink : krScenes[i]->links)
        {
            *(uint8_t*)&pBuffer[bufferOff++] = krLink.first;
            *(uint16_t*)&pBuffer[bufferOff]  = krLink.second;

            bufferOff += sizeof(uint16_t);
        }
    }

    WriteFile(SCENES_PATH, pBuffer, bufferOff);

    delete[] pBuffer;
}

void Storage::FactoryReset(void)
{
    Storage* pStorage;
    Pattern* pPattern;
    File     file;

    std::vector<std::shared_ptr<Pattern>> patternPtrs;
    std::shared_ptr<Pattern> patternPtr1;
    std::shared_ptr<Pattern> patternPtr2;
    std::shared_ptr<Pattern> patternPtr120FULL;
    std::shared_ptr<Pattern> patternPtr120MID;
    std::shared_ptr<Pattern> patternPtr120LOW;
    std::vector<SColor> colors;
    std::vector<SAnimation> anims;
    std::unordered_map<uint8_t, uint16_t> map;
    std::vector<std::shared_ptr<SScene>>  scenes;

    /* Create patterns */
    patternPtrs.push_back(std::make_shared<Pattern>(0, "P0"));
    patternPtrs.push_back(std::make_shared<Pattern>(1, "P1"));
    patternPtrs.push_back(std::make_shared<Pattern>(2, "P2"));
    patternPtrs.push_back(std::make_shared<Pattern>(3, "P3"));
    patternPtrs.push_back(std::make_shared<Pattern>(4, "P4"));
    patternPtrs.push_back(std::make_shared<Pattern>(5, "P5"));

    /* Pattern 0 */
    pPattern = patternPtrs[0].get();
    pPattern->SetBrightness(25);
    colors.push_back({
        .startIdx = 14,
        .endIdx   = 29,
        .startColorCode = 255,
        .endColorCode = 0
    });
    colors.push_back({
        .startIdx = 44,
        .endIdx   = 59,
        .startColorCode = 255,
        .endColorCode = 0
    });
    colors.push_back({
        .startIdx = 60,
        .endIdx   = 75,
        .startColorCode = 0,
        .endColorCode = 255
    });
    colors.push_back({
        .startIdx = 90,
        .endIdx   = 105,
        .startColorCode = 0,
        .endColorCode = 255
    });
    pPattern->SetColors(colors);
    anims.push_back({
        .type = ANIM_TRAIL,
        .startIdx = 0,
        .endIdx = 59,
        .param = 1
    });
    anims.push_back({
        .type = ANIM_TRAIL,
        .startIdx = 119,
        .endIdx = 60,
        .param = 1
    });
    pPattern->SetAnimations(anims);

    /* Pattern 1 */
    pPattern = patternPtrs[1].get();
    pPattern->SetBrightness(25);
    colors.clear();
    colors.push_back({
        .startIdx = 0,
        .endIdx   = 69,
        .startColorCode = 255 << 16,
        .endColorCode = 0
    });
    pPattern->SetColors(colors);
    anims.clear();
    anims.push_back({
        .type = ANIM_BREATH,
        .startIdx = 0,
        .endIdx = 34,
        .param = 1
    });
    pPattern->SetAnimations(anims);

    /* Pattern 2 */
    pPattern = patternPtrs[2].get();
    pPattern->SetBrightness(100);
    colors.clear();
    colors.push_back({
        .startIdx = 0,
        .endIdx   = 69,
        .startColorCode = 255 << 8,
        .endColorCode = 0
    });
    pPattern->SetColors(colors);
    anims.push_back({
        .type = ANIM_TRAIL,
        .startIdx = 0,
        .endIdx = 20,
        .param = 1
    });
    pPattern->SetAnimations(anims);

    /* Consumption test patterns */
    pPattern = patternPtrs[3].get();
    pPattern->SetBrightness(255);
    colors.clear();
    colors.push_back({
        .startIdx = 0,
        .endIdx   = 119,
        .startColorCode = 0xFFFFFFFF,
        .endColorCode = 0xFFFFFFFF
    });
    pPattern->SetColors(colors);

    pPattern = patternPtrs[4].get();
    pPattern->SetBrightness(150);
    colors.clear();
    colors.push_back({
        .startIdx = 0,
        .endIdx   = 119,
        .startColorCode = 0xFFFFFFFF,
        .endColorCode = 0xFFFFFFFF
    });
    pPattern->SetColors(colors);

    pPattern = patternPtrs[5].get();
    pPattern->SetBrightness(50);
    colors.clear();
    colors.push_back({
        .startIdx = 0,
        .endIdx   = 119,
        .startColorCode = 0xFFFFFFFF,
        .endColorCode = 0xFFFFFFFF
    });
    pPattern->SetColors(colors);

    /* Scenes */
    scenes.push_back(std::make_shared<SScene>());
    scenes.push_back(std::make_shared<SScene>());
    scenes.push_back(std::make_shared<SScene>());
    scenes.push_back(std::make_shared<SScene>());
    scenes.push_back(std::make_shared<SScene>());
    scenes.push_back(std::make_shared<SScene>());

    scenes[0]->name = "Scene0";
    scenes[0]->links.emplace(4, 0);
    scenes[0]->links.emplace(5, 1);

    scenes[1]->name = "Scene1";
    scenes[1]->links.emplace(4, 2);
    scenes[1]->links.emplace(5, 2);

    scenes[2]->name = "Scene120FULL";
    scenes[2]->links.emplace(4, 3);
    scenes[3]->name = "Scene120MID";
    scenes[3]->links.emplace(4, 4);
    scenes[4]->name = "Scene120LOW";
    scenes[4]->links.emplace(4, 5);
    scenes[5]->name = "Scene120OFF";

    SaveBrightness(255);
    SavePin("0000");
    SaveToken("1234567891113150");
    SavePatterns(patternPtrs);
    SaveScenes(scenes);
    SaveSelectedScene(1);

    Update(true);

    /* Create init file */
    LOG_DEBUG("Creating %s\n", INIT_FILE_PATH);
    file = SPIFFS.open(INIT_FILE_PATH, FILE_WRITE);
    close(file);

    LOG_INFO("Initialized flash\n");
}