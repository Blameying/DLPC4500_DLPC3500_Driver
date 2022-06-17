#pragma once

#include <QObject>
#include <functional>
#include <vector>
#include <utility>

class QDLP350_Driver: public QObject
{
    Q_OBJECT

public:
    enum TriggerMode {
        INTERNAL = 0,
        EXTERNAL = 1
    };

public:
    QDLP350_Driver(QObject *parent = Q_NULLPTR);
    ~QDLP350_Driver();

    bool open();
    bool isOpened();
    // TODO: 编写triggermode逻辑
    void setTriggerMode(QDLP350_Driver::TriggerMode mode);
    QDLP350_Driver::TriggerMode getTriggerMode();
    void sendSinglePage(int id, int exposure_time);
    void sendGroupPages(std::vector<int> ids, int exposure_time);
    void sendGroupPagesWithExp(std::vector<std::pair<int, int>> ids);
    void play();
    void pause();
    void stop();
    void playWithoutResult();
    void pauseWithoutResult();
    void stopWithoutResult();

private:
    void validate();
    void initConfiguration();
    void waitUntil(int delay_ms, int max_retry_times, std::function<void()> config, std::function<bool()> check);

private:
    TriggerMode triggerMode = TriggerMode::INTERNAL;
};