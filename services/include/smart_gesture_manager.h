/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef UI_APPEARANCE_SMART_GESTURE_MANAGER_H
#define UI_APPEARANCE_SMART_GESTURE_MANAGER_H

#include <functional>
#include <mutex>

#include "errors.h"
#include "nocopyable.h"

namespace OHOS::ArkUi::UiAppearance {
class SmartGestureManager final : public NoCopyable {
public:
    static SmartGestureManager &GetInstance();

    ErrCode Initialize(const std::function<void(bool, int32_t)>& updateCallback);

    ErrCode RegisterSettingDataObserver() const;

    void UpdateSmartGestureInitialValue();

private:
    enum SmartGestureMode {
        SMART_GESTURE_INVALID = -1,
        SMART_GESTURE_DISABLE = 0,
        SMART_GESTURE_AUTO = 1,
    };

    void LoadSettingDataObserversCallback();

    void UpdateSmartGestureValue(const std::string& key, int32_t userId);

    void OnChangeSmartGestureMode(SmartGestureMode mode, int32_t userId);

    std::pair<std::string, std::function<void(const std::string&, int32_t)>> observer_;

    std::mutex updateCallbackMutex_;
    std::function<void(bool, int32_t)> updateCallback_;
};
} // namespace OHOS::ArkUi::UiAppearance

#endif // UI_APPEARANCE_SMART_GESTURE_MANAGER_H
