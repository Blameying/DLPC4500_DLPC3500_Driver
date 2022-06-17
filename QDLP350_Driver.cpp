#include "QDLP350_Driver.h"
#include "dlpc350_common.h"
#include "dlpc350_api.h"
#include "dlpc350_usb.h"
#include <QThread>
#include <iostream>
#include <QDebug>
#include <QMessageBox>

QDLP350_Driver::QDLP350_Driver(QObject *parent)
    :QObject(parent)
{
    DLPC350_USB_Init();
}


bool QDLP350_Driver::open()
{
    this->waitUntil(1000, 10, [](){DLPC350_USB_Open();}, []()->bool{
        std::cout << "waiting connect" << std::endl;
        return DLPC350_USB_IsConnected();
    });
    this->initConfiguration();
    return true;
}

bool QDLP350_Driver::isOpened()
{
    return DLPC350_USB_IsConnected();
}

void QDLP350_Driver::initConfiguration()
{
    bool mode = false;
    unsigned int patMode;
    DLPC350_GetMode(&mode);
    if (mode == false) {
        this->waitUntil(100, 5, [](){ DLPC350_SetMode(true);}, [=]()->bool {
            bool mode = false;
            if (!DLPC350_USB_IsConnected()) {
                QMessageBox::critical(nullptr, tr("USB connect error!"), tr("Please check the usb connection, and then press the OK button"), QMessageBox::Ok);
                this->waitUntil(1000, 10, []() {DLPC350_USB_Open(); }, []()->bool {
                    std::cout << "waiting connect" << std::endl;
                    return DLPC350_USB_IsConnected();
                });
            }
            DLPC350_GetMode(&mode);
            return mode;
        });
    } else {
        DLPC350_GetPatternDisplay(&patMode);
        if (patMode != 0) {
            this->waitUntil(100, 5, [](){
                DLPC350_PatternDisplay(0);
            }, []()->bool {
                unsigned int patMode = 0;
                DLPC350_GetPatternDisplay(&patMode);
                if (patMode == 0) {
                    return true;
                }
                return false;
            });
        }
    }

    // set pattern sequence mode

    DLPC350_SetPatternDisplayMode(false);

    DLPC350_SetLedEnables(false, false, false, true);
    DLPC350_SetLEDPWMInvert(false);
    unsigned char RedCurrent      = 255-104;
    unsigned char GreenCurrent    = 255-135;
    unsigned char BlueCurrent     = 255-60;
    DLPC350_SetLedCurrents(RedCurrent, GreenCurrent, BlueCurrent);

    DLPC350_SetPatternTriggerMode(0x01);
}

void QDLP350_Driver::waitUntil(int delay_ms, int max_retry_times, std::function<void()> config, std::function<bool()> check)
{
    QThread::msleep(100);
    for (int i = 0; i < max_retry_times; i++) {
        config();
        QThread::msleep(delay_ms);
        bool ret = check();
        if (ret) {
            break;
        }
    }
}

void QDLP350_Driver::sendSinglePage(int id, int exposure_time)
{
    if (!DLPC350_USB_IsConnected()) {
        return;
    }

    if (id < 0 || id >= 24) {
        qDebug() << "QDLP350 pattern sequence id range error";
        return;
    }
    DLPC350_ClearPatLut();
    DLPC350_AddToPatLut(this->triggerMode, id, 1, 7, false, true, true, false);
    DLPC350_SetPatternConfig(1, true, 1, 1);
    DLPC350_SetExposure_FramePeriod(exposure_time, exposure_time);
    DLPC350_SetPatternTriggerMode(0x01);
    if (DLPC350_SendPatLut() < 0) {
        std::cerr << "QDLP350 send pat lut failed" << std::endl;
        return;
    }
    unsigned char value = 0;
    DLPC350_SendImageLut(&value, 1);
    this->validate();
    return;
}

void QDLP350_Driver::sendGroupPagesWithExp(std::vector<std::pair<int, int>> ids)
{
    if (!DLPC350_USB_IsConnected()) {
        return;
    }

    DLPC350_ClearExpLut();
    bool isFirst = true;
    for (auto pair : ids) {
        if (isFirst) { 
            // 第一张图要开启buf_swap, 否则会乱码
            // The first parameter should enable the buf_swap feature
            DLPC350_AddToExpLut(this->triggerMode, pair.first, 1, 7, false, false, true, false, pair.second, pair.second);
            isFirst = false;
        } else {
            DLPC350_AddToExpLut(this->triggerMode, pair.first, 1, 7, false, false, false, false, pair.second, pair.second);
        }
    }
    DLPC350_SetPatternDisplayMode(false);
    DLPC350_SetVarExpPatternConfig(ids.size(), 1, 1, true);
    DLPC350_SetPatternTriggerMode(0x03);
    if (DLPC350_SendVarExpPatLut() < 0) {
        std::cerr << "QDLP350 send pat lut failed" << std::endl;
        return;
    }
    unsigned char value = 0;
    DLPC350_SendVarExpImageLut(&value, 1);

    this->validate();

    return;
}


void QDLP350_Driver::sendGroupPages(std::vector<int> ids, int exposure_time)
{
    if (!DLPC350_USB_IsConnected()) {
        return;
    }

    DLPC350_ClearPatLut();
    bool isFirst = true;
    for (int id : ids) {
        if (isFirst) {
            // 第一张图要开启buf_swap, 否则会乱码
            // The first parameter should enable the buf_swap feature
            DLPC350_AddToPatLut(this->triggerMode, id, 1, 7, false, false, true, false);
            isFirst = false;
        } else {
            DLPC350_AddToPatLut(this->triggerMode, id, 1, 7, false, false, false, false);
        }
    }
    DLPC350_SetPatternDisplayMode(false);
    DLPC350_SetPatternConfig(ids.size(), true, 1, 1);
    DLPC350_SetExposure_FramePeriod(exposure_time, exposure_time);
    DLPC350_SetPatternTriggerMode(0x01);
    if (DLPC350_SendPatLut() < 0) {
        std::cerr << "QDLP350 send pat lut failed" << std::endl;
        return;
    }
    unsigned char value = 0;
    DLPC350_SendImageLut(&value, 1);

    this->validate();

    return;
}


void QDLP350_Driver::validate()
{
    unsigned int patMode = 0;
    if (DLPC350_GetPatternDisplay(&patMode) != 0 || patMode != 0) {
        this->waitUntil(100, 5, [](){ DLPC350_PatternDisplay(0); }, []()->bool {
            unsigned int patMode = 0;
            DLPC350_GetPatternDisplay(&patMode);
            if (patMode == 0) {
                return true;
            }
            return false;
        });
    }

    if (DLPC350_StartPatLutValidate()) {
        return;
    }

    this->waitUntil(10, 100, [](){}, []()->bool{
        bool ready = false;
        unsigned int status = 0;
        if (DLPC350_CheckPatLutValidate(&ready, &status) < 0) {
            return false;
        }
        if (ready) {
            return true;
        }
        return false;
    });
}

void QDLP350_Driver::play()
{
    this->waitUntil(10, 10, [](){ DLPC350_PatternDisplay(2); }, []()->bool {
        unsigned int patMode = 0;
        DLPC350_GetPatternDisplay(&patMode);
        if (patMode == 2) {
            return true;
        }
        return false;
    });
}

void QDLP350_Driver::playWithoutResult()
{
    DLPC350_PatternDisplay(2);
}

void QDLP350_Driver::pause()
{
    this->waitUntil(10, 10, [](){ DLPC350_PatternDisplay(1); }, []()->bool {
        unsigned int patMode = 0;
        DLPC350_GetPatternDisplay(&patMode);
        if (patMode == 1) {
            return true;
        }
        return false;
    });
}

void QDLP350_Driver::pauseWithoutResult()
{
    DLPC350_PatternDisplay(1);
}

void QDLP350_Driver::stop()
{
    this->waitUntil(10, 10, [](){ DLPC350_PatternDisplay(0); }, []()->bool {
        unsigned int patMode = 0;
        DLPC350_GetPatternDisplay(&patMode);
        if (patMode == 0) {
            return true;
        }
        return false;
    });
}

void QDLP350_Driver::stopWithoutResult()
{
    DLPC350_PatternDisplay(0);
}

QDLP350_Driver::~QDLP350_Driver()
{
    DLPC350_USB_Close();
    DLPC350_USB_Exit();
}

void QDLP350_Driver::setTriggerMode(QDLP350_Driver::TriggerMode mode)
{
    this->triggerMode = mode;
}

QDLP350_Driver::TriggerMode QDLP350_Driver::getTriggerMode()
{
    return this->triggerMode;
}
