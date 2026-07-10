/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "dark_mode_manager.h"

#include "setting_data_manager.h"
#include "ui_appearance_log.h"

namespace OHOS::ArkUi::UiAppearance {
namespace {
const std::string SETTING_DARK_MODE_MODE = "settings.uiappearance.darkmode_mode";
const std::string SETTING_DARK_MODE_START_TIME = "settings.uiappearance.darkmode_starttime";
const std::string SETTING_DARK_MODE_END_TIME = "settings.uiappearance.darkmode_endtime";
const std::string SETTING_DARK_MODE_SUN_SET = "settings.display.sun_set";
const std::string SETTING_DARK_MODE_SUN_RISE = "settings.display.sun_rise";
const static int32_t USER100 = 100;
constexpr int32_t MINUTE_TO_SECOND = 60;
constexpr int32_t OFFSET_SECONDS = 5;
}

DarkModeManager &DarkModeManager::GetInstance()
{
    static DarkModeManager instance;
    return instance;
}

ErrCode DarkModeManager::Initialize(const std::function<void(bool, int32_t)>& updateCallback)
{
    LoadSettingDataObserversCallback();
    updateCallback_ = updateCallback;
    return ERR_OK;
}

ErrCode DarkModeManager::LoadUserSettingData(
    const int32_t userId, const bool needUpdateCallback, bool &isDarkMode, const bool bootLoadFlag)
{
    return LoadUserSettingData(AccountContextHelper::CreateBaseContext(userId), needUpdateCallback, isDarkMode,
        bootLoadFlag);
}

ErrCode DarkModeManager::LoadUserSettingData(
    const AccountContext& context, const bool needUpdateCallback, bool &isDarkMode, const bool bootLoadFlag)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    auto getInt32Value = [&manager, &context](const std::string& baseKey, int32_t& value) {
        const std::string contextKey = AccountContextHelper::BuildSettingKey(baseKey, context);
        ErrCode code = manager.GetInt32ValueStrictly(contextKey, value, context.userId);
        if (code == ERR_OK || !AccountContextHelper::IsSubProfileContext(context)) {
            return code;
        }
        code = manager.GetInt32ValueStrictly(baseKey, value, context.userId);
        if (code == ERR_OK) {
            manager.SetStringValue(contextKey, std::to_string(value), context.userId, false);
        }
        return code;
    };
    int32_t darkMode = DARK_MODE_INVALID;
    getInt32Value(SETTING_DARK_MODE_MODE, darkMode);
    if (darkMode < DARK_MODE_INVALID || darkMode >= DARK_MODE_SIZE) {
        LOGE("dark mode out of range: %{public}d, context: %{public}s", darkMode,
            AccountContextHelper::ToString(context).c_str());
        darkMode = DARK_MODE_INVALID;
    }
    int32_t startTime = -1;
    getInt32Value(SETTING_DARK_MODE_START_TIME, startTime);
    int32_t endTime = -1;
    getInt32Value(SETTING_DARK_MODE_END_TIME, endTime);
    int32_t sunsetTime = SUNSET_TIME_DEFAULT;
    getInt32Value(SETTING_DARK_MODE_SUN_SET, sunsetTime);
    int32_t sunriseTime = SUNRISE_TIME_DEFAULT;
    getInt32Value(SETTING_DARK_MODE_SUN_RISE, sunriseTime);

    std::lock_guard lock(darkModeStatesMutex_);
    DarkModeState& state = darkModeStates_[context];
    state.settingMode = static_cast<DarkModeMode>(darkMode);
    state.settingStartTime = startTime;
    state.settingEndTime = endTime;
    state.settingSunsetTime = sunsetTime;
    state.settingSunriseTime = sunriseTime;
    LOGI("load user setting data, context: %{public}s, mode: %{public}d, start: %{public}d, end : %{public}d",
        AccountContextHelper::ToString(context).c_str(), darkMode, startTime, endTime);
    temporaryColorModeMgr_.InitData(context);
    if (temporaryColorModeMgr_.IsColorModeTemporary(context) &&
        temporaryColorModeMgr_.CheckTemporaryStateEffective(context) == false) {
        temporaryColorModeMgr_.SetColorModeNormal(context);
    }
    screenSwitchOperatorMgr_.ResetScreenOffOperateInfo();
    return OnStateChangeLocked(context, needUpdateCallback, isDarkMode, false, bootLoadFlag);
}

void DarkModeManager::NotifyDarkModeUpdate(const int32_t userId, const bool isDarkMode)
{
    NotifyDarkModeUpdate(AccountContextHelper::CreateBaseContext(userId), isDarkMode);
}

void DarkModeManager::NotifyDarkModeUpdate(const AccountContext& context, const bool isDarkMode)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    std::lock_guard lock(darkModeStatesMutex_);
    const DarkModeState& state = darkModeStates_[context];
    const std::string key = AccountContextHelper::BuildSettingKey(SETTING_DARK_MODE_MODE, context);
    if (isDarkMode) {
        if (state.settingMode == DARK_MODE_ALWAYS_LIGHT || state.settingMode == DARK_MODE_INVALID) {
            LOGI("notify change to always dark, context: %{public}s",
                AccountContextHelper::ToString(context).c_str());
            manager.SetStringValue(key, std::to_string(DARK_MODE_ALWAYS_DARK), context.userId);
        } // else no need to change
    } else {
        if (state.settingMode == DARK_MODE_ALWAYS_DARK || state.settingMode == DARK_MODE_INVALID) {
            LOGI("notify change to always light, context: %{public}s",
                AccountContextHelper::ToString(context).c_str());
            manager.SetStringValue(key, std::to_string(DARK_MODE_ALWAYS_LIGHT), context.userId);
        } // else no need to change
    }
}

void DarkModeManager::ScreenOnCallback()
{
    screenSwitchOperatorMgr_.SetScreenOn();
}

void DarkModeManager::ScreenOffCallback()
{
    screenSwitchOperatorMgr_.SetScreenOff();
    if (screenSwitchOperatorMgr_.HaveScreenOffOperate()) {
        bool switchToDark = false;
        int32_t userId = USER100;
        screenSwitchOperatorMgr_.GetScreenOffOperateInfo(switchToDark, userId);
        OnChangeDarkMode(
            switchToDark == true ? DarkModeMode::DARK_MODE_ALWAYS_DARK : DarkModeMode::DARK_MODE_ALWAYS_LIGHT,
            AccountContextHelper::CreateBaseContext(userId));
        screenSwitchOperatorMgr_.ResetScreenOffOperateInfo();
    }
}

ErrCode DarkModeManager::OnSwitchUser(const int32_t userId)
{
    return OnSwitchContext(AccountContextHelper::CreateBaseContext(userId));
}

ErrCode DarkModeManager::OnSwitchContext(const AccountContext& context)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    if (!manager.IsInitialized()) {
        ErrCode code = manager.Initialize();
        if (code != ERR_OK || manager.IsInitialized() == false) {
            LOGE("setting data manager is not initialized");
            return ERR_NO_INIT;
        }
    }

    if (context.userId <= INVALID_USER_ID) {
        LOGE("invalid context: %{public}s", AccountContextHelper::ToString(context).c_str());
        return ERR_INVALID_OPERATION;
    }

    std::lock_guard lock(settingDataObserversMutex_);
    if (settingDataObserversContext_.userId == INVALID_USER_ID && settingDataObserversUserId_ != INVALID_USER_ID) {
        settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(settingDataObserversUserId_);
    }
    if (settingDataObserversContext_.userId != INVALID_USER_ID) {
        LOGI("clear timers and unregister observers for context: %{public}s",
            AccountContextHelper::ToString(settingDataObserversContext_).c_str());
        alarmTimerManager_.ClearTimerByUserId(AccountContextHelper::BuildTimerKey(settingDataObserversContext_));
        UnregisterSettingDataObserversLocked(settingDataObserversContext_);
        settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(INVALID_USER_ID);
        settingDataObserversUserId_ = INVALID_USER_ID;
    }

    ErrCode code = RegisterSettingDataObserversLocked(context);
    settingDataObserversContext_ = context;
    settingDataObserversUserId_ = context.userId;
    return code;
}

void DarkModeManager::DoSwitchTemporaryColorMode(const int32_t userId, bool isDarkMode)
{
    DoSwitchTemporaryColorMode(AccountContextHelper::CreateBaseContext(userId), isDarkMode);
}

void DarkModeManager::DoSwitchTemporaryColorMode(const AccountContext& context, bool isDarkMode)
{
    if (IsDarkModeCustomAuto(context) || IsDarkModeSunsetSunrise(context)) {
        screenSwitchOperatorMgr_.ResetScreenOffOperateInfo();
        int32_t settingStartTime = 0;
        int32_t settingEndTime = 0;
        auto res = GetSettingTime(context, settingStartTime, settingEndTime);
        if (res == false) {
            LOGE("GetSettingTime faild context: %{public}s", AccountContextHelper::ToString(context).c_str());
            return;
        }
        if ((AlarmTimerManager::IsWithinTimeInterval(settingStartTime, settingEndTime) && isDarkMode == true) ||
            (!AlarmTimerManager::IsWithinTimeInterval(settingStartTime, settingEndTime) && isDarkMode == false)) {
            temporaryColorModeMgr_.SetColorModeNormal(context);
        } else {
            temporaryColorModeMgr_.SetColorModeTemporary(context);
        }
    }
}

void DarkModeManager::UpdateDarkModeSchedule(
    const DarkModeMode mode, const AccountContext& context, const bool resetTempColorModeFlag, const bool bootLoadFlag)
{
    screenSwitchOperatorMgr_.ResetScreenOffOperateInfo();
    if (resetTempColorModeFlag == true) {
        OnChangeDarkMode(mode, context);
        return;
    }

    if (screenSwitchOperatorMgr_.IsScreenOff() || bootLoadFlag) {
        if (temporaryColorModeMgr_.IsColorModeNormal(context) ||
            temporaryColorModeMgr_.CheckTemporaryStateEffective(context) == false) {
            OnChangeDarkMode(mode, context);
        }
        return;
    }

    if (temporaryColorModeMgr_.IsColorModeNormal(context) ||
        temporaryColorModeMgr_.CheckTemporaryStateEffective(context) == false) {
        screenSwitchOperatorMgr_.SetScreenOffOperateInfo(mode == DARK_MODE_ALWAYS_DARK, context.userId);
        LOGI("SetScreenOffOperateInfo context:%{public}s operate:%{public}d",
            AccountContextHelper::ToString(context).c_str(), static_cast<int32_t>(mode));
    }
}

ErrCode DarkModeManager::RestartTimer()
{
    std::lock_guard lock(darkModeStatesMutex_);
    AccountContext currentContext = settingDataObserversContext_;
    if (currentContext.userId == INVALID_USER_ID && settingDataObserversUserId_ != INVALID_USER_ID) {
        currentContext = AccountContextHelper::CreateBaseContext(settingDataObserversUserId_);
    }
    DarkModeMode mode = darkModeStates_[currentContext].settingMode;
    int32_t startTime = -1;
    int32_t endTime = -1;

    if (mode == DARK_MODE_SUNRISE_SUNSET) {
        startTime = darkModeStates_[currentContext].settingSunsetTime;
        endTime = darkModeStates_[currentContext].settingSunriseTime;
    } else if (mode == DARK_MODE_CUSTOM_AUTO) {
        startTime = darkModeStates_[currentContext].settingStartTime;
        endTime = darkModeStates_[currentContext].settingEndTime;
    } else {
        LOGD("no need to restart timer.");
        return ERR_OK;
    }

    if (AlarmTimerManager::IsWithinTimeInterval(startTime, endTime)) {
        UpdateDarkModeSchedule(DARK_MODE_ALWAYS_DARK, currentContext, false, false);
    } else {
        UpdateDarkModeSchedule(DARK_MODE_ALWAYS_LIGHT, currentContext, false, false);
    }
    return alarmTimerManager_.RestartAllTimer();
}

bool DarkModeManager::IsDarkModeCustomAuto(const AccountContext& context)
{
    std::lock_guard lock(darkModeStatesMutex_);
    return darkModeStates_[context].settingMode == DARK_MODE_CUSTOM_AUTO;
}

bool DarkModeManager::IsDarkModeSunsetSunrise(const AccountContext& context)
{
    std::lock_guard lock(darkModeStatesMutex_);
    return darkModeStates_[context].settingMode == DARK_MODE_SUNRISE_SUNSET;
}

bool DarkModeManager::GetSettingTime(const int32_t userId, int32_t& settingStartTime, int32_t& settingEndTime)
{
    return GetSettingTime(AccountContextHelper::CreateBaseContext(userId), settingStartTime, settingEndTime);
}

bool DarkModeManager::GetSettingTime(const AccountContext& context, int32_t& settingStartTime, int32_t& settingEndTime)
{
    std::lock_guard lock(darkModeStatesMutex_);
    auto it = darkModeStates_.find(context);
    if (it != darkModeStates_.end()) {
        if (it->second.settingMode == DARK_MODE_CUSTOM_AUTO) {
            settingStartTime = it->second.settingStartTime;
            settingEndTime = it->second.settingEndTime;
        } else {
            settingStartTime = it->second.settingSunsetTime;
            settingEndTime = it->second.settingSunriseTime;
        }
        return true;
    }
    return false;
}

bool DarkModeManager::IsColorModeNormal(const int32_t userId)
{
    return IsColorModeNormal(AccountContextHelper::CreateBaseContext(userId));
}

bool DarkModeManager::IsColorModeNormal(const AccountContext& context)
{
    return temporaryColorModeMgr_.IsColorModeNormal(context);
}

void DarkModeManager::Dump()
{
    {
        std::lock_guard observersGuard(settingDataObserversMutex_);
        LOGD("settingData observers size: %{public}zu, context: %{public}s",
            settingDataObservers_.size(), AccountContextHelper::ToString(settingDataObserversContext_).c_str());
    }

    std::lock_guard stateGuard(darkModeStatesMutex_);
    LOGD("darkModeStates size: %{public}zu", darkModeStates_.size());
    for (const auto& state : darkModeStates_) {
        LOGD("context: %{public}s, mode: %{public}d, start: %{public}d, end: %{public}d",
            AccountContextHelper::ToString(state.first).c_str(), state.second.settingMode,
            state.second.settingStartTime, state.second.settingEndTime);
    }

    alarmTimerManager_.Dump();
}

void DarkModeManager::LoadSettingDataObserversCallback()
{
    std::lock_guard lock(settingDataObserversMutex_);
    settingDataObservers_.clear();
    settingDataObservers_.emplace_back(SETTING_DARK_MODE_MODE, [&](const std::string& key,
        const AccountContext& context) {
        SettingDataDarkModeModeUpdateFunc(key, context);
    });
    settingDataObservers_.emplace_back(SETTING_DARK_MODE_START_TIME, [&](const std::string& key,
        const AccountContext& context) {
        SettingDataDarkModeStartTimeUpdateFunc(key, context);
    });
    settingDataObservers_.emplace_back(SETTING_DARK_MODE_END_TIME, [&](const std::string& key,
        const AccountContext& context) {
        SettingDataDarkModeEndTimeUpdateFunc(key, context);
    });
    settingDataObservers_.emplace_back(SETTING_DARK_MODE_SUN_SET, [&](const std::string& key,
        const AccountContext& context) {
        SettingDataDarkModeSunsetTimeUpdateFunc(key, context);
    });
    settingDataObservers_.emplace_back(SETTING_DARK_MODE_SUN_RISE, [&](const std::string& key,
        const AccountContext& context) {
        SettingDataDarkModeSunriseTimeUpdateFunc(key, context);
    });
}

ErrCode DarkModeManager::RegisterSettingDataObserversLocked(const AccountContext& context) const
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    size_t count = 0;
    for (const auto& observer : settingDataObservers_) {
        const std::string key = AccountContextHelper::BuildSettingKey(observer.first, context);
        auto updateFunc = [observer, context](const std::string& updateKey, int32_t userId) {
            observer.second(updateKey, context);
        };
        if (manager.RegisterObserver(key, updateFunc, context.userId) != ERR_OK) {
            count++;
        }
    }
    if (count != 0) {
        LOGE("setting data observers are not all initialized");
        return ERR_NO_INIT;
    }
    LOGD("setting data observers are all initialized");
    return ERR_OK;
}

void DarkModeManager::UnregisterSettingDataObserversLocked(const AccountContext& context) const
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    for (const auto& observer : settingDataObservers_) {
        manager.UnregisterObserver(AccountContextHelper::BuildSettingKey(observer.first, context), context.userId);
    }
}

void DarkModeManager::SettingDataDarkModeModeUpdateFunc(const std::string& key, const AccountContext& context)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    int32_t value = DARK_MODE_INVALID;
    ErrCode code = manager.GetInt32ValueStrictly(key, value, context.userId);
    if (code != ERR_OK) {
        LOGE("get dark mode value failed, key: %{public}s, context: %{public}s, code: %{public}d, set to default",
            key.c_str(), AccountContextHelper::ToString(context).c_str(), code);
        value = DARK_MODE_INVALID;
    }
    if (value < DARK_MODE_INVALID || value >= DARK_MODE_SIZE) {
        LOGE("dark mode value is invalid, key: %{public}s, context: %{public}s, value: %{public}d, set to default",
            key.c_str(), AccountContextHelper::ToString(context).c_str(), value);
        value = DARK_MODE_INVALID;
    }

    auto mode = static_cast<DarkModeMode>(value);
    std::lock_guard lock(darkModeStatesMutex_);
    LOGI("dark mode change, key: %{public}s, context: %{public}s, from %{public}d to %{public}d",
        key.c_str(), AccountContextHelper::ToString(context).c_str(), darkModeStates_[context].settingMode, value);
    darkModeStates_[context].settingMode = mode;
    bool isDarkMode = false;
    OnStateChangeLocked(context, true, isDarkMode, true, false);
}

void DarkModeManager::SettingDataDarkModeStartTimeUpdateFunc(const std::string& key, const AccountContext& context)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    int32_t value = -1;
    manager.GetInt32ValueStrictly(key, value, context.userId);
    std::lock_guard lock(darkModeStatesMutex_);
    LOGI("dark mode start time change, key: %{public}s, context: %{public}s, from %{public}d to %{public}d",
        key.c_str(), AccountContextHelper::ToString(context).c_str(), darkModeStates_[context].settingStartTime,
        value);
    darkModeStates_[context].settingStartTime = value;
    bool isDarkMode = false;
    OnStateChangeLocked(context, true, isDarkMode, true, false);
}

void DarkModeManager::SettingDataDarkModeEndTimeUpdateFunc(const std::string& key, const AccountContext& context)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    int32_t value = -1;
    manager.GetInt32ValueStrictly(key, value, context.userId);
    std::lock_guard lock(darkModeStatesMutex_);
    LOGI("dark mode end time change, key: %{public}s, context: %{public}s, from %{public}d to %{public}d",
        key.c_str(), AccountContextHelper::ToString(context).c_str(), darkModeStates_[context].settingEndTime, value);
    darkModeStates_[context].settingEndTime = value;
    bool isDarkMode = false;
    OnStateChangeLocked(context, true, isDarkMode, true, false);
}

void DarkModeManager::SettingDataDarkModeSunsetTimeUpdateFunc(const std::string& key, const AccountContext& context)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    int32_t value = SUNSET_TIME_DEFAULT;
    manager.GetInt32ValueStrictly(key, value, context.userId);
    std::lock_guard lock(darkModeStatesMutex_);
    LOGI("dark mode sunset time change, key: %{public}s, context: %{public}s, from %{public}d to %{public}d",
        key.c_str(), AccountContextHelper::ToString(context).c_str(), darkModeStates_[context].settingSunsetTime,
        value);
    if (value >= darkModeStates_[context].settingSunriseTime) {
        darkModeStates_[context].settingSunsetTime = SUNSET_TIME_DEFAULT;
        darkModeStates_[context].settingSunriseTime = SUNRISE_TIME_DEFAULT;
    } else {
        darkModeStates_[context].settingSunsetTime = value;
    }
    bool isDarkMode = false;
    OnStateChangeLocked(context, true, isDarkMode, false, false);
}

void DarkModeManager::SettingDataDarkModeSunriseTimeUpdateFunc(const std::string& key, const AccountContext& context)
{
    SettingDataManager& manager = SettingDataManager::GetInstance();
    int32_t value = SUNRISE_TIME_DEFAULT;
    manager.GetInt32ValueStrictly(key, value, context.userId);
    std::lock_guard lock(darkModeStatesMutex_);
    LOGI("dark mode sunrise time change, key: %{public}s, context: %{public}s, from %{public}d to %{public}d",
        key.c_str(), AccountContextHelper::ToString(context).c_str(), darkModeStates_[context].settingSunriseTime,
        value);
    if (value <= darkModeStates_[context].settingSunsetTime) {
        darkModeStates_[context].settingSunsetTime = SUNSET_TIME_DEFAULT;
        darkModeStates_[context].settingSunriseTime = SUNRISE_TIME_DEFAULT;
    } else {
        darkModeStates_[context].settingSunriseTime = value;
    }
    bool isDarkMode = false;
    OnStateChangeLocked(context, true, isDarkMode, false, false);
}

ErrCode DarkModeManager::OnStateChangeLocked(const AccountContext& context, const bool needUpdateCallback,
    bool& isDarkMode, const bool resetTempColorModeFlag, const bool bootLoadFlag)
{
    ErrCode code = ERR_OK;
    DarkModeState& state = darkModeStates_[context];
    switch (state.settingMode) {
        case DARK_MODE_ALWAYS_LIGHT:
        case DARK_MODE_ALWAYS_DARK:
            code = OnStateChangeToAllDayMode(
                context, state.settingMode, needUpdateCallback, isDarkMode, resetTempColorModeFlag, bootLoadFlag);
            break;
        case DARK_MODE_CUSTOM_AUTO:
        case DARK_MODE_SUNRISE_SUNSET:
            code = OnStateChangeToCustomAutoMode(context, state, needUpdateCallback, isDarkMode,
                resetTempColorModeFlag, bootLoadFlag);
            break;
        default:
            // do nothing
            code = ERR_INVALID_OPERATION;
            break;
    }
    return code;
}

ErrCode DarkModeManager::OnStateChangeToAllDayMode(const AccountContext& context, const DarkModeMode darkMode,
    const bool needUpdateCallback, bool& isDarkMode, const bool resetTempColorModeFlag, const bool bootLoadFlag)
{
    alarmTimerManager_.ClearTimerByUserId(AccountContextHelper::BuildTimerKey(context));
    isDarkMode = darkMode == DARK_MODE_ALWAYS_DARK;
    if (needUpdateCallback) {
        UpdateDarkModeSchedule(darkMode, context, resetTempColorModeFlag, bootLoadFlag);
    }
    return ERR_OK;
}

ErrCode DarkModeManager::OnStateChangeToCustomAutoMode(const AccountContext& context, const DarkModeState& state,
    const bool needUpdateCallback, bool& isDarkMode, const bool resetTempColorModeFlag, const bool bootLoadFlag)
{
    int32_t startTime = -1;
    int32_t endTime = -1;
    if (state.settingMode == DARK_MODE_SUNRISE_SUNSET) {
        startTime = state.settingSunsetTime;
        endTime = state.settingSunriseTime;
    } else {
        startTime = state.settingStartTime;
        endTime = state.settingEndTime;
    }

    ErrCode code = CreateOrUpdateTimers(startTime, endTime, context);
    if (code != ERR_OK) {
        alarmTimerManager_.ClearTimerByUserId(AccountContextHelper::BuildTimerKey(context));
        return code;
    }
    DarkModeMode mode = DARK_MODE_INVALID;
    if (AlarmTimerManager::IsWithinTimeInterval(startTime, endTime)) {
        isDarkMode = true;
        mode = DARK_MODE_ALWAYS_DARK;
    } else {
        isDarkMode = false;
        mode = DARK_MODE_ALWAYS_LIGHT;
    }

    if (needUpdateCallback) {
        UpdateDarkModeSchedule(mode, context, resetTempColorModeFlag, bootLoadFlag);
    }
    return ERR_OK;
}

void DarkModeManager::OnChangeDarkMode(const DarkModeMode mode, const AccountContext& context)
{
    if (!updateCallback_) {
        LOGE("no update callback, mode: %{public}d, context: %{public}s", mode,
            AccountContextHelper::ToString(context).c_str());
        return;
    }
    updateCallback_(mode == DARK_MODE_ALWAYS_DARK, context.userId);
    if (temporaryColorModeMgr_.IsColorModeTemporary(context)) {
        temporaryColorModeMgr_.SetColorModeNormal(context);
    }
}

ErrCode DarkModeManager::CreateOrUpdateTimers(int32_t startTime, int32_t endTime, const AccountContext& context)
{
    auto callbackSetColorMode = [startTime, endTime, context]() {
        LOGI("timer callback, startTime: %{public}d, endTime: %{public}d, context: %{public}s",
            startTime, endTime, AccountContextHelper::ToString(context).c_str());
        DarkModeMode colorMode = DarkModeMode::DARK_MODE_ALWAYS_LIGHT;
        ErrCode code = GetInstance().CheckTimerCallbackParams(startTime, endTime, context, colorMode);
        if (code != ERR_OK) {
            LOGE("timer callback, params check failed: %{public}d", code);
            return;
        }
        GetInstance().UpdateDarkModeSchedule(colorMode, context, false, false);
    };
    return alarmTimerManager_.SetScheduleTime(startTime, endTime, AccountContextHelper::BuildTimerKey(context),
        callbackSetColorMode, callbackSetColorMode);
}

ErrCode DarkModeManager::CreateOrUpdateTimers(int32_t startTime, int32_t endTime, int32_t userId)
{
    return CreateOrUpdateTimers(startTime, endTime, AccountContextHelper::CreateBaseContext(userId));
}

ErrCode DarkModeManager::CheckTimerCallbackParams(
    const int32_t startTime, const int32_t endTime, const AccountContext& context, DarkModeMode &darkMode)
{
    std::lock_guard lock(darkModeStatesMutex_);
    DarkModeState& state = darkModeStates_[context];
    if (state.settingMode == DARK_MODE_CUSTOM_AUTO) {
        if (state.settingStartTime != startTime) {
            LOGE("timer callback, param wrong, startTime: %{public}d, setting: %{public}d",
                startTime, state.settingStartTime);
            return ERR_INVALID_OPERATION;
        }
        if (state.settingEndTime != endTime) {
            LOGE("timer callback, param wrong, endTime: %{public}d, setting: %{public}d",
                endTime, state.settingEndTime);
            return ERR_INVALID_OPERATION;
        }
    } else if (state.settingMode == DARK_MODE_SUNRISE_SUNSET) {
        if (state.settingSunsetTime != startTime) {
            LOGE("timer callback, param wrong, sunsetTime: %{public}d, setting: %{public}d",
                startTime, state.settingSunsetTime);
            return ERR_INVALID_OPERATION;
        }
        if (state.settingSunriseTime != endTime) {
            LOGE("timer callback, param wrong, sunriseTime: %{public}d, setting: %{public}d",
                endTime, state.settingSunriseTime);
            return ERR_INVALID_OPERATION;
        }
    } else {
        LOGE("timer callback, param wrong, setting mode: %{public}d", state.settingMode);
        return ERR_INVALID_OPERATION;
    }

    int32_t currentTimeSeconds = 0;
    ErrCode code = GetCurrentTimeOfSeconds(currentTimeSeconds);
    if (code != ERR_OK) {
        LOGE("GetCurrentTimeOfSeconds error: %{public}d", code);
        return code;
    }
    bool inDarkIntervalFlag = CheckCurrentTimeInDarkInterval(startTime, endTime, currentTimeSeconds);
    darkMode = (inDarkIntervalFlag ? DarkModeMode::DARK_MODE_ALWAYS_DARK : DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    return ERR_OK;
}


ErrCode DarkModeManager::GetCurrentTimeOfSeconds(int32_t &seconds)
{
    std::time_t timestamp = std::time(nullptr);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("fail to get timestamp");
        return ERR_INVALID_OPERATION;
    }
    std::tm *nowTime = std::localtime(&timestamp);
    if (nowTime == nullptr) {
        LOGE("fail to get localtime");
        return ERR_INVALID_OPERATION;
    }
    seconds = static_cast<int32_t>(
        nowTime->tm_hour * HOUR_TO_MINUTE * MINUTE_TO_SECOND + nowTime->tm_min * MINUTE_TO_SECOND + nowTime->tm_sec);
    return ERR_OK;
}

bool DarkModeManager::CheckCurrentTimeInDarkInterval(
    const int32_t startTime, const int32_t endTime, const int32_t currentTimeSeconds)
{
    //Due to 1-second offset in the timer trigger, so subtract OFFSET_SECONDS
    if (endTime <= DAY_TO_MINUTE) {
        if ((startTime * MINUTE_TO_SECOND - OFFSET_SECONDS < currentTimeSeconds) &&
            (currentTimeSeconds < endTime * MINUTE_TO_SECOND - OFFSET_SECONDS)) {
            return true;
        }
    } else {
        if ((currentTimeSeconds < (endTime - DAY_TO_MINUTE) * MINUTE_TO_SECOND - OFFSET_SECONDS) ||
            (currentTimeSeconds > startTime * MINUTE_TO_SECOND - OFFSET_SECONDS)) {
            return true;
        }
    }
    return false;
}
} // namespace OHOS::ArkUi::UiAppearance
