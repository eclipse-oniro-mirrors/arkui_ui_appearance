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

#include "smart_gesture_manager.h"

#include "setting_data_manager.h"
#include "ui_appearance_log.h"

namespace OHOS::ArkUi::UiAppearance {
namespace {
const std::string SETTING_SMART_GESTURE_SWITCH = "persist.gesture.smart_gesture_enable";
}

SmartGestureManager &SmartGestureManager::GetInstance()
{
    static SmartGestureManager instance;
    return instance;
}

ErrCode SmartGestureManager::Initialize(const std::function<void(bool, int32_t)>& updateCallback)
{
    LoadSettingDataObserversCallback();
    {
        std::lock_guard lock(updateCallbackMutex_);
        updateCallback_ = updateCallback;
    }
    return ERR_OK;
}

void SmartGestureManager::LoadSettingDataObserversCallback()
{
    observer_ = std::make_pair(SETTING_SMART_GESTURE_SWITCH, [&](const std::string& key, int32_t userId) {
        UpdateSmartGestureValue(key, userId);
    });
}

ErrCode SmartGestureManager::RegisterSettingDataObserver() const
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    if (manager.RegisterObserver(observer_.first, observer_.second, INVALID_USER_ID) != ERR_OK) {
        LOGE("setting data observer is not initialized");
        return ERR_NO_INIT;
    }
    LOGD("setting data observer is initialized");
    return ERR_OK;
}

void SmartGestureManager::UpdateSmartGestureValue(const std::string& key, const int32_t userId)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    int32_t value = SMART_GESTURE_INVALID;
    ErrCode code = manager.GetInt32ValueStrictly(key, value, userId);
    if (code != ERR_OK) {
        LOGE("get smart gesture value failed, key: %{public}s, userId: %{public}d, code: %{public}d",
            key.c_str(), userId, code);
        return;
    }

    if (value < SMART_GESTURE_DISABLE || value > SMART_GESTURE_AUTO) {
        LOGE("smart gesture value is invalid, key: %{public}s, userId: %{public}d, value: %{public}d",
            key.c_str(), userId, value);
        return;
    }

    auto mode = static_cast<SmartGestureMode>(value);
    LOGD("smart gesture mode change, key: %{public}s, userId: %{public}d, value: %{public}d",
        key.c_str(), userId, value);
    OnChangeSmartGestureMode(mode, userId);
}

void SmartGestureManager::UpdateSmartGestureInitialValue()
{
    UpdateSmartGestureValue(SETTING_SMART_GESTURE_SWITCH, INVALID_USER_ID);
}

void SmartGestureManager::OnChangeSmartGestureMode(const SmartGestureMode mode, const int32_t userId)
{
    std::function<void(bool, int32_t)> updateCallback;
    {
        std::lock_guard lock(updateCallbackMutex_);
        updateCallback = updateCallback_;
    }
    if (!updateCallback) {
        LOGE("no update callback, mode: %{public}d, userId: %{public}d", mode, userId);
        return;
    }
    updateCallback(mode == SMART_GESTURE_AUTO, userId);
}
} // namespace OHOS::ArkUi::UiAppearance
