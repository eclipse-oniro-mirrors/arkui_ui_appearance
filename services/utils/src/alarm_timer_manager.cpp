/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "alarm_timer_manager.h"

#include <array>
#include <cstdint>
#include <ctime>
#include <sys/time.h>
#include <cinttypes>
#include "ui_appearance_log.h"

namespace OHOS {
namespace ArkUi::UiAppearance {
constexpr int32_t DAY_TO_SECOND = 24 * 60 * 60;
constexpr int32_t DAY_TO_MINUTE = 24 * 60;
constexpr int32_t SECOND_TO_MILLI = 1000;
constexpr int32_t HOUR_TO_MINUTE = 60;
constexpr int32_t MINUTE_TO_SECOND = 60;
constexpr int32_t TIMER_TYPE_EXACT = 2 | 4;
constexpr int32_t START_INDEX = 0;
constexpr int32_t END_INDEX = 1;
const std::string START_TIMER_NAME = "dark_mode_start_timer";
const std::string END_TIMER_NAME = "dark_mode_end_timer";

ErrCode AlarmTimerManager::SetScheduleTime(const uint64_t startTime, const uint64_t endTime,
    const uint64_t userId, const std::function<void()>& startCallback, const std::function<void()>& endCallback)
{
    std::lock_guard<std::mutex> lock(timerMapMutex_);
    if (!IsValidScheduleTime(startTime, endTime)) {
        LOGE("userId:%{public}" PRIu64 ", start %{public}" PRIu64 ", end %{public}" PRIu64,
            userId, startTime, endTime);
        return ERR_INVALID_VALUE;
    }
    RecordInitialSetupTime(startTime, endTime, userId);
    std::array<uint64_t, TRIGGER_ARRAY_SIZE> triggerTimeInterval = { 0, 0 };
    SetTimerTriggerTime(startTime, endTime, triggerTimeInterval);
    LOGI("userId: %{public}" PRIu64 ", in %{public}" PRIu64 " %{public}" PRIu64 ", trigger %{public}" PRIu64
         " %{public}" PRIu64, userId, startTime, endTime, triggerTimeInterval[START_INDEX],
        triggerTimeInterval[END_INDEX]);
    SetTimer(0, userId, triggerTimeInterval[START_INDEX], startCallback);
    SetTimer(1, userId, triggerTimeInterval[END_INDEX], endCallback);
    if (timerIdMap_[userId][START_INDEX] == 0 || timerIdMap_[userId][END_INDEX] == 0) {
        LOGE("set timer fail, timerId: %{public}" PRIu64 " %{public}" PRIu64,
            timerIdMap_[userId][START_INDEX], timerIdMap_[userId][END_INDEX]);
        return ERR_INVALID_OPERATION;
    }
    LOGI("set timer success, timerId: %{public}" PRIu64 " %{public}" PRIu64,
        timerIdMap_[userId][START_INDEX], timerIdMap_[userId][END_INDEX]);
    return ERR_OK;
}

bool AlarmTimerManager::IsValidScheduleTime(const uint64_t startTime, const uint64_t endTime)
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

void AlarmTimerManager::SetTimerTriggerTime(const uint64_t startTime, const uint64_t endTime,
    std::array<uint64_t, TRIGGER_ARRAY_SIZE>& triggerTimeInterval)
{
    std::time_t timestamp = std::time(nullptr);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("fail to get timestamp");
    }
    std::tm* nowTime = std::localtime(&timestamp);
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
            triggerTimeInterval = { startTimestamp, endTimestamp - step };
        } else {
            triggerTimeInterval = { startTimestamp, endTimestamp };
        }
    } else if (curTimestamp >= endTimestamp) {
        triggerTimeInterval = { startTimestamp + step, endTimestamp + step };
    } else {
        triggerTimeInterval = { startTimestamp + step, endTimestamp };
    }
}

void AlarmTimerManager::Dump()
{
    std::lock_guard<std::mutex> lock(timerMapMutex_);
    LOGD("timerIdMap size: %{public}zu", timerIdMap_.size());
    for (const auto& it : timerIdMap_) {
        LOGD("userId:%{public}" PRIu64 ", start %{public}" PRIu64 ", end %{public}" PRIu64,
            it.first, it.second[0], it.second[1]);
    }
    LOGD("initialSetupTimeMap size: %{public}zu", initialSetupTimeMap_.size());
    for (const auto& it : initialSetupTimeMap_) {
        LOGD("userId:%{public}" PRIu64 ", start %{public}" PRIu64 ", end %{public}" PRIu64,
            it.first, it.second[0], it.second[1]);
    }
}

void AlarmTimerManager::SetTimer(const int8_t index, const uint64_t userId, const uint64_t time,
    const std::function<void()>& callback)
{
    LOGD("SetTimer %{public}d %{public}" PRIu64 " %{public}" PRIu64, index, userId, time);
    if (timerIdMap_.find(userId) == timerIdMap_.end()) {
        LOGW("timerIdMap_ find userId null");
        std::array<uint64_t, TRIGGER_ARRAY_SIZE> timerIds = { 0, 0 };
        timerIdMap_[userId] = timerIds;
    }

    if (timerIdMap_[userId][index] > 0) {
        timerIdMap_[userId][index] =
            UpdateTimer(timerIdMap_[userId][index], time, callback, index == 0 ? START_TIMER_NAME : END_TIMER_NAME);
    } else {
        timerIdMap_[userId][index] = InitTimer(time, callback, index == 0 ? START_TIMER_NAME : END_TIMER_NAME);
    }
}

uint64_t AlarmTimerManager::InitTimer(const uint64_t time, const std::function<void()>& callback,
    const std::string timerName)
{
    auto timerInfo = std::make_shared<AlarmTimer>();
    timerInfo->SetType(TIMER_TYPE_EXACT);
    timerInfo->SetRepeat(true);
    timerInfo->SetInterval(DAY_TO_SECOND * SECOND_TO_MILLI);
    timerInfo->SetCallbackInfo(callback);
    timerInfo->SetName(timerName);
    uint64_t id = static_cast<uint64_t>(MiscServices::TimeServiceClient::GetInstance()->CreateTimer(timerInfo));
    if (id <= 0) {
        LOGE("fail to create timer %{public}" PRIu64, id);
        return 0;
    }
    bool ret = MiscServices::TimeServiceClient::GetInstance()->StartTimer(id, time);
    if (!ret) {
        LOGE("fail to StartTimer timer %{public}" PRIu64, id);
        ClearTimer(id);
        return 0;
    }
    LOGI("success to StartTimer timer %{public}" PRIu64, id);
    return id;
}

void AlarmTimerManager::ClearTimer(const uint64_t id)
{
    if (id <= 0) {
        LOGE("id <= 0: %{public}" PRIu64, id);
        return;
    }

    bool ret = MiscServices::TimeServiceClient::GetInstance()->DestroyTimer(id);
    if (!ret) {
        LOGE("fail to DestroyTimer timer %{public}" PRIu64, id);
    }
    LOGI("success to DestroyTimer timer %{public}" PRIu64, id);
}

uint64_t AlarmTimerManager::UpdateTimer(const uint64_t id, const uint64_t time,
    const std::function<void()>& callback, const std::string timerName)
{
    ClearTimer(id);
    return InitTimer(time, callback, timerName);
}

void AlarmTimerManager::ClearTimerByUserId(const uint64_t userId)
{
    std::lock_guard<std::mutex> lock(timerMapMutex_);
    if (timerIdMap_.find(userId) == timerIdMap_.end()) {
        LOGE("timerIdMap_ fail to find Timer: %{public}" PRIu64, userId);
        return;
    }

    ClearTimer(timerIdMap_[userId][START_INDEX]);
    ClearTimer(timerIdMap_[userId][END_INDEX]);
    timerIdMap_.erase(userId);

    if (initialSetupTimeMap_.find(userId) == initialSetupTimeMap_.end()) {
        LOGE("initialSetupTimeMap_ fail to find Timer: %{public}" PRIu64, userId);
    }
    initialSetupTimeMap_.erase(userId);
}

bool AlarmTimerManager::IsWithinTimeInterval(const uint64_t startTime, const uint64_t endTime)
{
    LOGI("IsWithinTimeInterval startTime: %{public}" PRIu64 " endTime: %{public}" PRIu64,
        startTime, endTime);
    std::time_t timestamp = std::time(nullptr);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("fail to get timestamp");
        return false;
    }
    std::tm* nowTime = std::localtime(&timestamp);
    uint32_t totalMinutes{ 0 };
    if (nowTime != nullptr) {
        totalMinutes = static_cast<uint32_t>(nowTime->tm_hour * HOUR_TO_MINUTE + nowTime->tm_min);
    }

    if (endTime <= DAY_TO_MINUTE) {
        return (startTime <= totalMinutes && totalMinutes < endTime);
    } else {
        if ((endTime - DAY_TO_MINUTE) <= totalMinutes && totalMinutes < startTime) {
            return false;
        }
        return true;
    }
}

void AlarmTimerManager::RecordInitialSetupTime(const uint64_t startTime, const uint64_t endTime,
    const uint64_t userId)
{
    std::array<uint64_t, TRIGGER_ARRAY_SIZE> initialSetupTime = { startTime, endTime };
    initialSetupTimeMap_[userId] = initialSetupTime;
}

bool AlarmTimerManager::RestartTimerByUserId(const uint64_t userId)
{
    if (userId == 0) {
        LOGE("userId == 0");
        return false;
    }

    if (timerIdMap_.find(userId) == timerIdMap_.end()
        || initialSetupTimeMap_.find(userId) == initialSetupTimeMap_.end()) {
        LOGE("initialSetupTimeMap_ or timerIdMap_ fail to find Timer: %{public}" PRIu64, userId);
        return false;
    }

    LOGI("RestartTimerByUserId userId: %{public}" PRIu64, userId);
    std::array<uint64_t, TRIGGER_ARRAY_SIZE> triggerTimeInterval = { 0, 0 };
    SetTimerTriggerTime(initialSetupTimeMap_[userId][START_INDEX],
        initialSetupTimeMap_[userId][END_INDEX], triggerTimeInterval);

    RestartTimerByTimerId(timerIdMap_[userId][START_INDEX], triggerTimeInterval[START_INDEX]);
    RestartTimerByTimerId(timerIdMap_[userId][END_INDEX], triggerTimeInterval[END_INDEX]);

    return true;
}

void AlarmTimerManager::RestartTimerByTimerId(const uint64_t timerId, const uint64_t time)
{
    LOGI("RestartTimerByTimerId timerId: %{public}" PRIu64 " timer: %{public}" PRIu64, timerId, time);
    MiscServices::TimeServiceClient::GetInstance()->StopTimer(timerId);
    MiscServices::TimeServiceClient::GetInstance()->StartTimer(timerId, time);
}

bool AlarmTimerManager::RestartAllTimer()
{
    std::lock_guard<std::mutex> lock(timerMapMutex_);
    bool res = true;
    for (const auto& pair : timerIdMap_) {
        uint64_t userId = pair.first;
        if (userId == 0) {
            LOGE("userId == 0: %{public}" PRIu64, userId);
            continue;
        }
        res = res && RestartTimerByUserId(userId);
    }

    return res;
}
} // namespace ArkUi::UiAppearance
} // namespace OHOS
