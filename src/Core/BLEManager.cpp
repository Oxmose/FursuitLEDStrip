/*******************************************************************************
 * @file BLEManager.cpp
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/04/2024
 *
 * @version 1.0
 *
 * @brief Bluetooth low energy manager.
 *
 * @details This file provides the bluetooth low energy manager services.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <BLEDevice.h> /* BLE Device Services*/
#include <BLEUtils.h>  /* BLE Untils Services*/
#include <BLEServer.h> /* BLE Server Services*/
#include <HWLayer.h>   /* HW layer */
#include <version.h>   /* Versioning */
#include <SystemState.h> /* System state services */
#include <Logger.h>      /* Logger */
#include <StripsManager.h> /* Strips manager */

/* Header File */
#include <BLEManager.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define MAIN_SERVICE_UUID               "d3e63261-287a-41a5-a421-ad0a81157af9"

/* Per device characteristics */
#define HW_VERSION_CHARACTERISTIC_UUID      "997ca8f9-abe6-4db1-b01a-768d9d405226" // OK
#define SW_VERSION_CHARACTERISTIC_UUID      "20a14f57-d375-44c8-a9c1-470521110471" // OK
#define SET_TOKEN_CHARACTERISTIC_UUID       "02b31496-6fda-48a5-a16e-ddb727919774" // OK
#define GET_STRIPS_CHARACTERISTIC_UUID      "2d3a8ac3-49d3-4ec1-a9ff-837dd2ff190f" // OK
#define GET_BATTERY_CHARACTERISTIC_UUID     "96d94fcc-ced5-48b4-b065-8c946747335e" // OK
#define BRIGHTNESS_CHARACTERISTIC_UUID      "83670c18-c3fd-4334-bc4b-2f9a8b70a3cf" // OK
#define MANAGE_PATTERNS_CHARACTERISTIC_UUID "ff957108-a010-4dff-8cc2-1600f48045c3"
#define MANAGE_SCENES_CHARACTERISTIC_UUID   "40325d79-46c1-4d7d-a71f-edfbe27b98d1" // OK
#define SET_SCENE_CHARACTERISTIC_UUID       "d5d97123-28bf-466b-9d73-2cf3f056bae0" // OK

#define SET_BRIGHTNESS_COMMAND_SIZE (BLE_TOCKEN_SIZE + sizeof(uint8_t))
#define SET_TOKEN_COMMAND_SIZE      (BLE_TOCKEN_SIZE + BLE_TOCKEN_SIZE)
#define SET_SCENE_COMMAND_SIZE      (BLE_TOCKEN_SIZE + sizeof(uint8_t))

#define BLE_CMD_SCENE_MGT_ADD 0
#define BLE_CMD_SCENE_MGT_REM 1
#define BLE_CMD_SCENE_MGT_UPD 2
#define BLE_CMD_SCENE_MGT_CNT 3
#define BLE_CMD_SCENE_MGT_GET 4

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

class BrightnessCallback: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic* pBrightnessCharacteristic)
    {
        uint8_t      value;
        uint8_t*     data;
        SystemState* pSysState;
        BLEManager*  pBle;

        pSysState = SystemState::GetInstance();
        pBle      = BLEManager::GetInstance();
        if(pBrightnessCharacteristic->getLength() ==
           SET_BRIGHTNESS_COMMAND_SIZE)
        {
            data = pBrightnessCharacteristic->getData();
            if(pBle->ValidateToken((char*)data))
            {
                value = *(data + BLE_TOCKEN_SIZE);
                /* Request value change and ack */
                LOG_INFO("New brightness request: %d\n", value);
                pSysState->SetBrightness(value);
            }
            else
            {
                LOG_ERROR("Invalid BLE Token\n");
            }

        }
        else
        {
            LOG_ERROR("Incorrect data length in brightness callback.\n");
        }
        value = pSysState->GetBrightness();
        pBrightnessCharacteristic->setValue(&value, 1);
    }
};

class SetTokenCallback: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic* pSetTokenCharacteristic)
    {
        char*        value;
        char         newToken[BLE_TOCKEN_SIZE + 1];
        SystemState* pSysState;
        BLEManager*  pBle;

        if(pSetTokenCharacteristic->getLength() ==
           SET_TOKEN_COMMAND_SIZE)
        {
            pBle  = BLEManager::GetInstance();
            value = (char*)pSetTokenCharacteristic->getData();
            if(pBle->ValidateToken(value))
            {
                strncpy(newToken, value + BLE_TOCKEN_SIZE, BLE_TOCKEN_SIZE);
                newToken[BLE_TOCKEN_SIZE] = 0;

                /* Request value change and ack */
                LOG_INFO("New token request: %s\n", newToken);
                pSysState = SystemState::GetInstance();
                pSysState->SetBLEToken(newToken);
            }
            else
            {
                LOG_ERROR("Invalid BLE Token\n");
            }

        }
        else
        {
            LOG_ERROR("Incorrect data length in set token callback.\n");
        }
    }
};

class ManagePatternsCallback: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic* pManagePatternsCharacteristic)
    {
        LOG_INFO("Not implemented\n");
    }
};

class ManageSceneCallback: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic* pManageSceneCharacteristic)
    {
        uint8_t*       data;
        BLEManager*    pBle;

        pBle = BLEManager::GetInstance();

        data = pManageSceneCharacteristic->getData();
        if(pBle->ValidateToken((char*)data) == false)
        {
            LOG_ERROR("Invalid BLE Token\n");
            return;
        }

        data = data + BLE_TOCKEN_SIZE;

        /* Get the command */
        switch(*data)
        {
            case BLE_CMD_SCENE_MGT_ADD:
                onAdd(data + sizeof(uint8_t), pManageSceneCharacteristic);
                break;
            case BLE_CMD_SCENE_MGT_REM:
                onRemove(data + sizeof(uint8_t), pManageSceneCharacteristic);
                break;
            case BLE_CMD_SCENE_MGT_UPD:
                onUpdate(data + sizeof(uint8_t), pManageSceneCharacteristic);
                break;
            case BLE_CMD_SCENE_MGT_CNT:
                onGetCount(data + sizeof(uint8_t), pManageSceneCharacteristic);
                break;
            case BLE_CMD_SCENE_MGT_GET:
                onGet(data + sizeof(uint8_t), pManageSceneCharacteristic);
                break;
            default:
                LOG_ERROR("Unknown command %d\n", *data);
        }
    }

    void onAdd(const uint8_t* kpData,
               BLECharacteristic* pManageSceneCharacteristic) const
    {
        uint8_t        retValue;
        StripsManager* pStripManager;

        std::shared_ptr<SScene> newScene;

        pStripManager = StripsManager::GetInstance();

        newScene = DeserializeScene(kpData);
        retValue = pStripManager->AddScene(newScene);

        pManageSceneCharacteristic->setValue(&retValue, sizeof(uint8_t));
    }

    void onRemove(const uint8_t* kpData,
                  BLECharacteristic* pManageSceneCharacteristic) const
    {
        StripsManager* pStripManager;
        bool           result;

        pStripManager = StripsManager::GetInstance();

        result = pStripManager->RemoveScene(*kpData);

        pManageSceneCharacteristic->setValue((uint8_t*)&result, sizeof(bool));
    }

    void onUpdate(const uint8_t* kpData,
                  BLECharacteristic* pManageSceneCharacteristic) const
    {
        uint8_t        retValue;
        StripsManager* pStripManager;

        std::shared_ptr<SScene> newScene;

        pStripManager = StripsManager::GetInstance();

        /* Check if the scene exists */
        if(*kpData < pStripManager->GetSceneCount())
        {
            newScene = DeserializeScene(kpData + sizeof(uint8_t));
            retValue = pStripManager->UpdateScene(*kpData, newScene);
        }
        else
        {
            retValue = 0;
        }

        pManageSceneCharacteristic->setValue(&retValue, sizeof(uint8_t));
    }

    void onGetCount(const uint8_t* kpData,
                    BLECharacteristic* pManageSceneCharacteristic) const
    {
        uint8_t pBuffer;

        pBuffer = StripsManager::GetInstance()->GetSceneCount();

        pManageSceneCharacteristic->setValue(&pBuffer, sizeof(uint8_t));
    }

    void onGet(const uint8_t* kpData,
               BLECharacteristic* pManageSceneCharacteristic) const
    {
        size_t         bufferSize;
        uint8_t        error;
        uint8_t*       pBuffer;
        const SScene*  pkScene;
        StripsManager* pStripManager;

        pStripManager = StripsManager::GetInstance();

        pStripManager->Lock();

        pkScene = StripsManager::GetInstance()->GetSceneInfo(*kpData);
        if(pkScene == nullptr)
        {
            error = -1;
            LOG_ERROR("Requested info for unknown scene %d\n", *kpData);
            pManageSceneCharacteristic->setValue(&error, sizeof(uint8_t));
            return;
        }

        /* Get the required buffer size */
        bufferSize = sizeof(uint8_t) +
                     sizeof(uint8_t) +
                     pkScene->name.size() +
                     sizeof(uint8_t) +
                     pkScene->links.size() *
                      (sizeof(uint8_t) + sizeof(uint16_t));
        pBuffer = new uint8_t[bufferSize];

        SerializeScene(pkScene, *kpData, pBuffer);

        pStripManager->Unlock();

        pManageSceneCharacteristic->setValue(pBuffer, bufferSize);

        delete[] pBuffer;
    }

    void SerializeScene(const SScene* pkScene,
                        const uint8_t kId,
                        uint8_t* pBuffer) const
    {
        size_t buffOffset;
        size_t sizeProp;

        buffOffset = 0;

        /* Write ID */
        *(uint8_t*)&pBuffer[buffOffset++] = kId;

        /* Write name and its size */
        sizeProp = pkScene->name.size();
        *(uint8_t*)&pBuffer[buffOffset++] = sizeProp;
        memcpy(&pBuffer[buffOffset], pkScene->name.c_str(), sizeProp);
        buffOffset += sizeProp;

        /* Write nb links and all links */
        sizeProp = pkScene->links.size();
        *(uint8_t*)&pBuffer[buffOffset++] = sizeProp;

        for(const std::pair<uint8_t, uint16_t>& krLink : pkScene->links)
        {
            *(uint8_t*)&pBuffer[buffOffset++] = krLink.first;
            *(uint16_t*)&pBuffer[buffOffset] = krLink.second;

            buffOffset += sizeof(uint16_t);
        }
    }

    std::shared_ptr<SScene> DeserializeScene(const uint8_t* pBuffer) const
    {
        size_t   buffOffset;
        size_t   sizeProp;
        uint8_t  stripId;
        uint16_t patternId;

        std::shared_ptr<SScene> scenePtr;

        scenePtr = std::make_shared<SScene>();

        buffOffset = 0;

        /* Get name size and name */
        sizeProp = *(uint8_t*)&pBuffer[buffOffset++];

        scenePtr->name = std::string(&pBuffer[buffOffset],
                                     &pBuffer[buffOffset] + sizeProp);
        buffOffset += sizeProp;

        /* Get nb links and for each, all them */
        sizeProp = *(uint8_t*)&pBuffer[buffOffset++];
        while(sizeProp > 0)
        {
            stripId   = *(uint8_t*)&pBuffer[buffOffset++];
            patternId = *(uint16_t*)&pBuffer[buffOffset];
            buffOffset += sizeof(uint16_t);

            scenePtr->links.emplace(stripId, patternId);

            --sizeProp;
        }

        return scenePtr;
    }
};

class SetSceneCallback: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic* pSetSceneCharacteristic)
    {
        uint8_t        value;
        uint8_t*       data;
        StripsManager* pStripManager;
        BLEManager*    pBle;

        pStripManager = StripsManager::GetInstance();
        pBle          = BLEManager::GetInstance();
        if(pSetSceneCharacteristic->getLength() ==
           SET_SCENE_COMMAND_SIZE)
        {
            data = pSetSceneCharacteristic->getData();
            if(pBle->ValidateToken((char*)data))
            {
                value = *(data + BLE_TOCKEN_SIZE);
                /* Request value change and ack */
                LOG_INFO("New scene select request: %d\n", value);
                pStripManager->SelectScene(value);
            }
            else
            {
                LOG_ERROR("Invalid BLE Token\n");
            }

        }
        else
        {
            LOG_ERROR("Incorrect data length in select scene callback.\n");
        }
        value = pStripManager->GetSelectedScene();
        pSetSceneCharacteristic->setValue(&value, 1);
    }
};

class ServerCallback: public BLEServerCallbacks
{
    void onDisconnect(BLEServer* pServer)
    {
        pServer->startAdvertising();
    }
};

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
BLEManager* BLEManager::PINSTANCE_ = nullptr;

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

BLEManager* BLEManager::GetInstance(void)
{
    if (BLEManager::PINSTANCE_ == nullptr)
    {
        BLEManager::PINSTANCE_ = new BLEManager();
    }

    return BLEManager::PINSTANCE_;
}

void BLEManager::Update(void)
{
    SystemState* pSysState;
    uint8_t      brightness;
    uint8_t      batteryPercent;

    pSysState = SystemState::GetInstance();

    /* Update values */
    brightness = pSysState->GetBrightness();
    batteryPercent = pSysState->GetBatteryPercent();

    pCharacteristicBrightness_->setValue(&brightness, sizeof(uint8_t));
    pCharacteristicBattery_->setValue(&batteryPercent, sizeof(uint8_t));
}

bool BLEManager::ValidateToken(const char* kpToken) const
{
    if(isInit_)
    {
        return strncmp(kpToken,
                       SystemState::GetInstance()->GetBLEToken(),
                       BLE_TOCKEN_SIZE) == 0;
    }
    return false;
}

BLEManager::BLEManager(void)
{
    isInit_ = false;
    Init();
}

void BLEManager::Init(void)
{
    uint8_t  value;
    uint8_t* pBuffer;
    size_t   bufferSize;

    SystemState*      pSysState;
    StripsManager*    pStripManager;
    StripsInfoTable_t stripInfoTable;

    if(isInit_)
    {
        return;
    }

    pSysState = SystemState::GetInstance();
    pStripManager = StripsManager::GetInstance();

    /* Initialize the server and services */
    BLEDevice::init(HWLayer::GetHWUID());
    pServer_ = BLEDevice::createServer();
    pMainService_ = pServer_->createService(BLEUUID(MAIN_SERVICE_UUID), 30U, 0);

    /* Setup server callback */
    pServer_->setCallbacks(new ServerCallback());

    /* Setup the VERSION characteristics */
    pCharacteristicHWersion_ = pMainService_->createCharacteristic(
                                            HW_VERSION_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ
                                        );
    pCharacteristicHWersion_->setValue(PROTO_REV);
    pCharacteristicSWersion_ = pMainService_->createCharacteristic(
                                            SW_VERSION_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ
                                        );
    pCharacteristicSWersion_->setValue(VERSION);

    /* Setup the TOKEN characteristic */
    pCharacteristicSetToken_ = pMainService_->createCharacteristic(
                                            SET_TOKEN_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
    pCharacteristicSetToken_->setCallbacks(new SetTokenCallback());

    /* Setup the BATTERY characteristic */
    pCharacteristicBattery_ = pMainService_->createCharacteristic(
                                            GET_BATTERY_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ
                                        );
    value = pSysState->GetBatteryPercent();
    pCharacteristicBattery_->setValue(&value, sizeof(uint8_t));

    /* Setup the BRIGHTNESS characteristic */
    pCharacteristicBrightness_ = pMainService_->createCharacteristic(
                                            BRIGHTNESS_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ |
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
    value = pSysState->GetBrightness();
    pCharacteristicBrightness_->setValue(&value, sizeof(uint8_t));
    pCharacteristicBrightness_->setCallbacks(new BrightnessCallback());

    /* Setup the STRIPS characteristic */
    pCharacteristicGetStrips_ = pMainService_->createCharacteristic(
                                            GET_STRIPS_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ
                                        );
    pStripManager->GetStripsInfo(stripInfoTable);
    pBuffer = SerializeStripsInfo(stripInfoTable, bufferSize);
    pCharacteristicGetStrips_->setValue(pBuffer, bufferSize);
    delete[] pBuffer;

    /* Setup the PATTERN characteristics */
    pCharacteristicManagePatterns_ = pMainService_->createCharacteristic(
                                            MANAGE_PATTERNS_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ |
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
    value = 0;
    pCharacteristicManagePatterns_->setValue(&value, sizeof(uint8_t));
    pCharacteristicManagePatterns_->setCallbacks(new ManagePatternsCallback());

    /* Setup the SCENE characteristics */
    pCharacteristicManageScenes_ = pMainService_->createCharacteristic(
                                            MANAGE_SCENES_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ |
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
    value = 0;
    pCharacteristicManageScenes_->setValue(&value, sizeof(uint8_t));
    pCharacteristicManageScenes_->setCallbacks(new ManageSceneCallback());

    pCharacteristicSetScene_ = pMainService_->createCharacteristic(
                                            SET_SCENE_CHARACTERISTIC_UUID,
                                            BLECharacteristic::PROPERTY_READ |
                                            BLECharacteristic::PROPERTY_WRITE
                                        );
    pCharacteristicSetScene_->setCallbacks(new SetSceneCallback());
    value = pStripManager->GetSelectedScene();
    pCharacteristicSetScene_->setValue(&value, sizeof(uint8_t));

    /* Start the services */
    pMainService_->start();

    /* Start advertising */
    pAdvertising_ = BLEDevice::getAdvertising();
    pAdvertising_->addServiceUUID(MAIN_SERVICE_UUID);
    pAdvertising_->setScanResponse(true);
    pAdvertising_->setMinPreferred(0x06);
    pAdvertising_->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    isInit_ = true;

    LOG_INFO("BLE Manager Initialized.\n");
}


uint8_t* BLEManager::SerializeStripsInfo(StripsInfoTable_t& rInfoTbl,
                                         size_t& rSize) const
{
    uint8_t*          pBuffer;
    off_t             offset;

    /* Get the buffer size */
    rSize = 0;
    for(std::shared_ptr<SStripInfo> stripInfo : rInfoTbl)
    {
        rSize += sizeof(gpio_num_t) +
                 sizeof(uint16_t) +
                 sizeof(bool) +
                 stripInfo->name.size() +
                 1;
    }
    rSize += sizeof(size_t);

    /* Copy buffer */
    offset = 0;
    pBuffer = new uint8_t[rSize];
    memcpy(pBuffer + offset, &rSize, sizeof(size_t));
    offset += sizeof(size_t);
    for(std::shared_ptr<SStripInfo> stripInfo : rInfoTbl)
    {
        LOG_DEBUG("Getting info for strip %s.\n", stripInfo->name.c_str());
        memcpy(pBuffer + offset, &stripInfo->ctrlGPIO, sizeof(gpio_num_t));
        offset += sizeof(gpio_num_t);
        memcpy(pBuffer + offset, &stripInfo->numLed, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy(pBuffer + offset, &stripInfo->isEnabled, sizeof(bool));
        offset += sizeof(bool);
        memcpy(pBuffer + offset,
               stripInfo->name.c_str(),
               stripInfo->name.size());
        offset += stripInfo->name.size();

        /* Terminate strip info */
        pBuffer[offset++] = 0;
    }

    LOG_DEBUG("Done\n");

    return pBuffer;
}