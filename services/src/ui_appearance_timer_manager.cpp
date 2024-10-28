/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ui_appearance_timer_manager.h"
#include <array>
#include <cstdint>
#include <ctime>
#include <memory>
#include <sys/time.h>
#include <cinttypes>

#include "alarm_timer.h"
#include "ui_appearance_log.h"
#include "ui_appearance_ability_interface.h"

namespace OHOS {
namespace ArkUi::UiAppearance {

const int DAY_TO_SECOND = 24 * 60 * 60;
const int DAY_TO_MINUTE = 24 * 60;
const int SECOND_TO_MILLI = 1000;
const int HOUR_TO_MINUTE = 60;
const int MINUTE_TO_SECOND = 60;
const int TIMER_TYPE_EXACT = 2 | 4;
const int START_INDEX = 0;
const int END_INDEX = 1;

uint32_t UiAppearanceTimerManager::SetScheduleTime(
    const uint64_t startTime, const uint64_t endTime, const uint32_t userId,
    std::function<void()> startCallback, std::function<void()> endCallback)
{
    LOGI("SetTimerTriggerTime");
    if (!IsVaildScheduleTime(startTime, endTime)) {
        return UiAppearanceAbilityInterface::INVALID_ARG;
    }
    RecordInitialSetupTime(startTime, endTime, userId);
    std::array<uint64_t, TRIGGER_ARRAY_SIZE> triggerTimeInterval = {0, 0};
    SetTimerTriggerTime(startTime, endTime, triggerTimeInterval);
    LOGI("SetTimerTriggerTime %{public}" PRIu64" %{public}" PRIu64 "",
        triggerTimeInterval[START_INDEX], triggerTimeInterval[END_INDEX]);
    SetTimer(0, userId, triggerTimeInterval[START_INDEX], startCallback);
    SetTimer(1, userId, triggerTimeInterval[END_INDEX], endCallback);
    return timerIdMap_[userId][START_INDEX] > 0 && timerIdMap_[userId][END_INDEX] > 0 ?
        UiAppearanceAbilityInterface::SUCCEEDED : UiAppearanceAbilityInterface::SYS_ERR;
}

bool UiAppearanceTimerManager::IsVaildScheduleTime (const uint64_t startTime, const uint64_t endTime)
{
    if (startTime >= endTime) {
        LOGE("startTime >= endTime");
        return false;
    }

    if (startTime >= DAY_TO_MINUTE) {
        LOGE("startTime >= DAY_TO_MINUTE");
        return false;
    }

    if (endTime >= DAY_TO_MINUTE + startTime) {
        LOGE("endTime >= DAY_TO_MINUTE + startTime");
        return false;
    }

    return true;
}

void UiAppearanceTimerManager::SetTimerTriggerTime(const uint64_t startTime, const uint64_t endTime,
    std::array<uint64_t, TRIGGER_ARRAY_SIZE> &triggerTimeInterval)
{
    LOGI("SetTimerTriggerTime");
    std::time_t timestamp = std::time(nullptr);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("fail to get timestamp");
    }
    std::tm *nowTime = std::localtime(&timestamp);
    if (nowTime != nullptr) {
        nowTime->tm_hour = 0;
        nowTime->tm_min = 0;
        nowTime->tm_sec = 0;
    }
    std::time_t now_zero = std::mktime(nowTime);
    uint64_t curTimestamp = static_cast<uint64_t>(timestamp * SECOND_TO_MILLI);
    uint64_t zeroTimestamp = static_cast<uint64_t>(now_zero * SECOND_TO_MILLI);
    uint64_t startTimestamp = zeroTimestamp + startTime * MINUTE_TO_SECOND * SECOND_TO_MILLI;
    uint64_t endTimestamp = zeroTimestamp + endTime * MINUTE_TO_SECOND * SECOND_TO_MILLI;

    uint64_t step = DAY_TO_SECOND * SECOND_TO_MILLI;
    if (curTimestamp <= startTimestamp) {
        if (curTimestamp < endTimestamp - step) {
            triggerTimeInterval = {startTimestamp, endTimestamp - step};
        } else {
            triggerTimeInterval = {startTimestamp, endTimestamp};
        }
    } else if (curTimestamp >= endTimestamp) {
        triggerTimeInterval = {startTimestamp + step, endTimestamp + step};
    } else {
        triggerTimeInterval = {startTimestamp + step, endTimestamp};
    }
}

void UiAppearanceTimerManager::SetTimer(const int8_t index, const uint32_t userId, const uint64_t time,
    std::function<void()> callback)
{
    LOGI("SetDarkModeTimer %{public}d %{public}d %{public}" PRIu64"", index, userId, time);
    if (timerIdMap_.find(userId) == timerIdMap_.end()) {
        std::array<uint64_t, TRIGGER_ARRAY_SIZE> timerIds = {0, 0};
        timerIdMap_[userId] = timerIds;
    }

    if (timerIdMap_[userId][index] > 0) {
        timerIdMap_[userId][index] = UpdateTimer(timerIdMap_[userId][index], time, callback);
    } else {
        timerIdMap_[userId][index] = InitTimer(time, callback);
    }
}

uint64_t UiAppearanceTimerManager::InitTimer(const uint64_t time, std::function<void()> callback)
{
    auto timerInfo = std::make_shared<AlarmTimer>();
    timerInfo->SetType(TIMER_TYPE_EXACT);
    timerInfo->SetRepeat(true);
    timerInfo->SetInterval(DAY_TO_SECOND * SECOND_TO_MILLI);
    timerInfo->SetCallbackInfo(callback);
    uint64_t id = static_cast<uint64_t>(MiscServices::TimeServiceClient::GetInstance()->CreateTimer(timerInfo));
    if (id <= 0) {
        LOGE("fail to create timer %{public}" PRIu64"", id);
        return 0;
    }
    bool ret = MiscServices::TimeServiceClient::GetInstance()->StartTimer(id, time);
    if (!ret) {
        LOGE("fail to StartTimer timer %{public}" PRIu64"", id);
        ClearTimer(id);
        return 0;
    }
    return id;
}

void UiAppearanceTimerManager::ClearTimer(const uint64_t id)
{
    if (id <= 0) {
        LOGE("id <= 0: %{public}" PRIu64"", id);
        return;
    }

    bool ret = MiscServices::TimeServiceClient::GetInstance()->DestroyTimer(id);
    if (!ret) {
        LOGE("fail to DestroyTimer timer %{public}" PRIu64"", id);
    }
}

uint64_t UiAppearanceTimerManager::UpdateTimer(const uint64_t id, const uint64_t time, std::function<void()> callback)
{
    ClearTimer(id);
    return InitTimer(time, callback);
}

void UiAppearanceTimerManager::ClearTimerByUserId(const uint64_t userId)
{
    if (timerIdMap_.find(userId) == timerIdMap_.end()) {
        LOGE("timerIdMap_ fail to find Timer: %{public}" PRIu64"", userId);
        return;
    }

    ClearTimer(timerIdMap_[userId][START_INDEX]);
    ClearTimer(timerIdMap_[userId][END_INDEX]);
    timerIdMap_.erase(userId);

    if (initialSetupTimeMap_.find(userId) == initialSetupTimeMap_.end()) {
        LOGE("initialSetupTimeMap_ fail to find Timer: %{public}" PRIu64"", userId);
    }
    initialSetupTimeMap_.erase(userId);
}

bool UiAppearanceTimerManager::IsWithinTimeInterval(const uint64_t startTime, const uint64_t endTime)
{
    std::time_t timestamp = std::time(nullptr);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("fail to get timestamp");
        return false;
    }
    std::tm *nowTime = std::localtime(&timestamp);
    uint32_t totalMinutes{ 0 };
    if (nowTime != nullptr) {
        totalMinutes = static_cast<uint32_t>(nowTime->tm_hour * HOUR_TO_MINUTE + nowTime->tm_min);
    }

    if (endTime <= DAY_TO_MINUTE) {
        if (startTime <= totalMinutes && totalMinutes < endTime) {
            LOGI("inner");
            return true;
        }
        LOGI("outter");
        return false;
    } else {
        if ((endTime - DAY_TO_MINUTE) <= totalMinutes && totalMinutes < startTime) {
            LOGI("outter");
            return false;
        }
        LOGI("inner");
        return true;
    }
}

void UiAppearanceTimerManager::RecordInitialSetupTime(const uint64_t startTime,
    const uint64_t endTime, const uint32_t userId)
{
    std::array<uint64_t, TRIGGER_ARRAY_SIZE> initialSetupTime = {startTime, endTime};
    initialSetupTimeMap_[userId] = initialSetupTime;
}

bool UiAppearanceTimerManager::RestartTimerByUserId(const uint64_t userId)
{
    if (userId == 0) {
        return RestartAllTimer();
    }

    if (timerIdMap_.find(userId) == timerIdMap_.end()
        || initialSetupTimeMap_.find(userId) == initialSetupTimeMap_.end()) {
        LOGE("initialSetupTimeMap_ or timerIdMap_ fail to find Timer: %{public}" PRIu64"", userId);
        return false;
    }

    LOGI("RestartTimerByUserId userId: %{public}" PRIu64"", userId);
    std::array<uint64_t, TRIGGER_ARRAY_SIZE> triggerTimeInterval = {0, 0};
    SetTimerTriggerTime(initialSetupTimeMap_[userId][START_INDEX],
        initialSetupTimeMap_[userId][END_INDEX], triggerTimeInterval);

    RestartTimerByTimerId(timerIdMap_[userId][START_INDEX], triggerTimeInterval[START_INDEX]);
    RestartTimerByTimerId(timerIdMap_[userId][END_INDEX], triggerTimeInterval[END_INDEX]);

    return true;
}

void UiAppearanceTimerManager::RestartTimerByTimerId(const uint64_t timerId, const uint64_t time)
{
    LOGI("RestartTimerByTimerId timerId: %{public}" PRIu64" timer: %{public}" PRIu64"", timerId, time);
    MiscServices::TimeServiceClient::GetInstance()->StopTimer(timerId);
    MiscServices::TimeServiceClient::GetInstance()->StartTimer(timerId, time);
}

bool UiAppearanceTimerManager::RestartAllTimer()
{
    LOGI("RestartAllTimer");

    bool res = true;

    for (const std::pair<uint64_t, std::array<uint64_t, TRIGGER_ARRAY_SIZE>>& pair : timerIdMap_) {
        uint64_t userId = pair.first;
        if (userId == 0) {
            LOGE("userId == 0: %{public}" PRIu64"", userId);
            continue;
        }
        res = res && RestartTimerByUserId(userId);
    }

    return res;
}

} // namespace ArkUi::UiAppearance
} // namespace OHOS