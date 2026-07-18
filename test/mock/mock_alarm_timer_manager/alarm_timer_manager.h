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

#ifndef UI_APPEARANCE_MOCK_UTILS_ALARM_TIMER_MANAGER_H
#define UI_APPEARANCE_MOCK_UTILS_ALARM_TIMER_MANAGER_H

#include <array>
#include <functional>
#include <gmock/gmock.h>

#include "errors.h"

namespace OHOS::ArkUi::UiAppearance {
constexpr uint32_t TRIGGER_ARRAY_SIZE = 2;

class AlarmTimerManager {
public:
    AlarmTimerManager() = default;

    virtual ~AlarmTimerManager() = default;

    static AlarmTimerManager &GetInstance()
    {
        static AlarmTimerManager instance_;
        return instance_;
    }

    static bool IsWithinTimeInterval(uint64_t startTime, uint64_t endTime)
    {
        return GetInstance().MockIsWithinTimeInterval(startTime, endTime);
    }

    MOCK_METHOD(ErrCode, SetScheduleTime, (uint64_t startTime, uint64_t endTime, uint64_t userId,
        const std::function<void()>& startCallback, const std::function<void()>& endCallback));
    MOCK_METHOD(void, ClearTimerByUserId, (uint64_t userId));
    MOCK_METHOD(bool, RestartAllTimer, ());
    MOCK_METHOD(void, Dump, (), (const));
    MOCK_METHOD(bool, MockIsWithinTimeInterval, (uint64_t startTime, uint64_t endTime));
};
} // namespace OHOS::ArkUi::UiAppearance
#endif // UI_APPEARANCE_MOCK_UTILS_ALARM_TIMER_MANAGER_H
