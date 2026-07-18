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

#ifndef UI_APPEARANCE_UTILS_ALARM_TIMER_MANAGER_H
#define UI_APPEARANCE_UTILS_ALARM_TIMER_MANAGER_H

#include <array>
#include <map>
#include <cstdint>

#include "alarm_timer.h"
#include "errors.h"

namespace OHOS::ArkUi::UiAppearance {
constexpr uint32_t TRIGGER_ARRAY_SIZE = 2;

class AlarmTimerManager {
public:
    AlarmTimerManager() = default;

    virtual ~AlarmTimerManager() = default;

    ErrCode SetScheduleTime(uint64_t startTime, uint64_t endTime, uint64_t userId,
        const std::function<void()>& startCallback, const std::function<void()>& endCallback);

    void ClearTimerByUserId(uint64_t userId);

    bool RestartAllTimer();

    static bool IsValidScheduleTime(uint64_t startTime, uint64_t endTime);

    static bool IsWithinTimeInterval(uint64_t startTime, uint64_t endTime);

    static void SetTimerTriggerTime(uint64_t startTime, uint64_t endTime,
        std::array<uint64_t, TRIGGER_ARRAY_SIZE>& triggerTimeInterval);

    void Dump();

private:
    std::map<uint64_t, std::array<uint64_t, TRIGGER_ARRAY_SIZE>> timerIdMap_;
    std::map<uint64_t, std::array<uint64_t, TRIGGER_ARRAY_SIZE>> initialSetupTimeMap_;
    std::mutex timerMapMutex_;

    static void RestartTimerByTimerId(uint64_t timerId, uint64_t time);

    static uint64_t InitTimer(uint64_t time, const std::function<void()>& callback, const std::string timerName);

    static uint64_t UpdateTimer(uint64_t id, uint64_t time,
        const std::function<void()>& callback, const std::string timerName);

    static void ClearTimer(uint64_t id);

    void RecordInitialSetupTime(uint64_t startTime, uint64_t endTime, uint64_t userId);

    void SetTimer(int8_t index, uint64_t userId, uint64_t time, const std::function<void()>& callback);

    bool RestartTimerByUserId(uint64_t userId);
};
} // namespace OHOS::ArkUi::UiAppearance
#endif // UI_APPEARANCE_UTILS_ALARM_TIMER_MANAGER_H
