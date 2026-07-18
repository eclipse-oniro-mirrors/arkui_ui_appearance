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

#ifndef UI_APPEARANCE_DARK_MODE_MANAGER_H
#define UI_APPEARANCE_DARK_MODE_MANAGER_H

#include <functional>
#include <list>
#include <map>
#include <mutex>

#include "account_context.h"
#include "errors.h"
#include "nocopyable.h"
#include "alarm_timer_manager.h"
#include "dark_mode_temp_state_manager.h"
#include "screen_switch_operator_manager.h"

namespace OHOS::ArkUi::UiAppearance {
constexpr int32_t HOUR_TO_MINUTE = 60;
constexpr int32_t DAY_TO_MINUTE = 24 * 60;
constexpr int32_t SUNSET_TIME_DEFAULT = 18 * HOUR_TO_MINUTE;
constexpr int32_t SUNRISE_TIME_DEFAULT = 7 * HOUR_TO_MINUTE + DAY_TO_MINUTE;
class DarkModeManager final : public NoCopyable {
public:
    static DarkModeManager &GetInstance();

    ErrCode Initialize(const std::function<void(bool, int32_t)>& updateCallback);

    ErrCode LoadUserSettingData(int32_t userId, bool needUpdateCallback, bool &isDarkMode, const bool bootLoadFlag);
    ErrCode LoadUserSettingData(const AccountContext& context, bool needUpdateCallback, bool &isDarkMode,
        const bool bootLoadFlag);

    void NotifyDarkModeUpdate(int32_t userId, bool isDarkMode);
    void NotifyDarkModeUpdate(const AccountContext& context, bool isDarkMode);

    ErrCode OnSwitchUser(int32_t userId);
    ErrCode OnSwitchContext(const AccountContext& context);

    void ScreenOnCallback();

    void ScreenOffCallback();

    ErrCode RestartTimer();

    void Dump();

    bool GetSettingTime(const int32_t userId, int32_t& settingStartTime, int32_t& settingEndTime);
    bool GetSettingTime(const AccountContext& context, int32_t& settingStartTime, int32_t& settingEndTime);

    bool IsColorModeNormal(const int32_t userId);
    bool IsColorModeNormal(const AccountContext& context);

    void DoSwitchTemporaryColorMode(const int32_t userId, bool isDarkMode);
    void DoSwitchTemporaryColorMode(const AccountContext& context, bool isDarkMode);

    static ErrCode GetCurrentTimeOfSeconds(int32_t &seconds);

    static bool CheckCurrentTimeInDarkInterval(
        const int32_t startTime, const int32_t endTime, const int32_t seconds);
private:
    enum DarkModeMode {
        DARK_MODE_INVALID = -1,
        DARK_MODE_ALWAYS_LIGHT = 0,
        DARK_MODE_ALWAYS_DARK = 1,
        DARK_MODE_CUSTOM_AUTO = 2,
        DARK_MODE_SUNRISE_SUNSET = 3,
        DARK_MODE_SIZE,
    };

    struct DarkModeState {
        DarkModeMode settingMode = DARK_MODE_INVALID;
        int32_t settingStartTime = -1;
        int32_t settingEndTime = -1;
        int32_t settingSunsetTime = SUNSET_TIME_DEFAULT;   // Default sunset Time 6pm
        int32_t settingSunriseTime = SUNRISE_TIME_DEFAULT; // Default sunrise time: 7am the next day
    };

    void LoadSettingDataObserversCallback();

    ErrCode RegisterSettingDataObserversLocked(const AccountContext& context) const;

    void UnregisterSettingDataObserversLocked(const AccountContext& context) const;

    void SettingDataDarkModeModeUpdateFunc(const std::string& key, const AccountContext& context);

    void SettingDataDarkModeStartTimeUpdateFunc(const std::string& key, const AccountContext& context);

    void SettingDataDarkModeEndTimeUpdateFunc(const std::string& key, const AccountContext& context);

    void SettingDataDarkModeSunsetTimeUpdateFunc(const std::string& key, const AccountContext& context);

    void SettingDataDarkModeSunriseTimeUpdateFunc(const std::string& key, const AccountContext& context);

    ErrCode OnStateChangeLocked(const AccountContext& context, bool needUpdateCallback, bool& isDarkMode,
        const bool resetTempColorModeFlag, const bool bootLoadFlag);

    ErrCode OnStateChangeToAllDayMode(const AccountContext& context, DarkModeMode darkMode, bool needUpdateCallback,
        bool& isDarkMode, const bool resetTempColorModeFlag, const bool bootLoadFlag);

    ErrCode OnStateChangeToCustomAutoMode(const AccountContext& context, const DarkModeState& state,
        bool needUpdateCallback, bool& isDarkMode, const bool resetTempColorModeFlag, const bool bootLoadFlag);

    void OnChangeDarkMode(DarkModeMode mode, const AccountContext& context);

    ErrCode CreateOrUpdateTimers(int32_t startTime, int32_t endTime, const AccountContext& context);
    ErrCode CreateOrUpdateTimers(int32_t startTime, int32_t endTime, int32_t userId);

    ErrCode CheckTimerCallbackParams(int32_t startTime, int32_t endTime, const AccountContext& context,
        DarkModeMode &darkMode);

    void UpdateDarkModeSchedule(const DarkModeMode isDarkMode, const AccountContext& context,
        const bool resetTempColorModeFlag, const bool bootLoadFlag);

    bool IsDarkModeCustomAuto(const AccountContext& context);

    bool IsDarkModeSunsetSunrise(const AccountContext& context);

    std::mutex settingDataObserversMutex_;
    std::list<std::pair<std::string, std::function<void(const std::string&, const AccountContext&)>>>
        settingDataObservers_;
    AccountContext settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(-1);
    // Kept for legacy userId-only paths and tests; new logic should use settingDataObserversContext_.
    int32_t settingDataObserversUserId_ = -1;

    AlarmTimerManager alarmTimerManager_;
    std::mutex darkModeStatesMutex_;
    std::map<AccountContext, DarkModeState> darkModeStates_;

    std::function<void(bool, int32_t)> updateCallback_;

    TemporaryColorModeManager temporaryColorModeMgr_;
    ScreenSwitchOperatorManager screenSwitchOperatorMgr_;
};
} // namespace OHOS::ArkUi::UiAppearance

#endif // UI_APPEARANCE_DARK_MODE_MANAGER_H
