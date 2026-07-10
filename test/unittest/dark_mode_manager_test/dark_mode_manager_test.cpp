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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// mock
#include "alarm_timer_manager.h"
#include "setting_data_manager.h"

#define private public
#include "dark_mode_manager.h"
#undef private

using namespace testing;
using namespace testing::ext;

namespace OHOS::ArkUi::UiAppearance {
namespace {
const std::string SETTING_DARK_MODE_MODE = "settings.uiappearance.darkmode_mode";
const std::string SETTING_DARK_MODE_START_TIME = "settings.uiappearance.darkmode_starttime";
const std::string SETTING_DARK_MODE_END_TIME = "settings.uiappearance.darkmode_endtime";
const std::string SETTING_DARK_MODE_SUN_SET = "settings.display.sun_set";
const std::string SETTING_DARK_MODE_SUN_RISE = "settings.display.sun_rise";
constexpr int32_t SETTING_NUM = 5;
constexpr int32_t TEST_USER100 = 100;
constexpr int32_t TEST_USER101 = 101;
constexpr int32_t TEST_USER1 = 1;
constexpr ErrCode TEST_ERROR = 123456;
constexpr int32_t TEST_DARK_MODE_STATE_NEGATIVE_TWO = -2;
constexpr int32_t TEST_DARK_MODE_STATE_NEGATIVE_ONE = -1;
constexpr int32_t TEST_DARK_MODE_STATE_ZERO = 0;
constexpr int32_t TEST_DARK_MODE_STATE_ONE = 1;
constexpr int32_t TEST_DARK_MODE_STATE_TWO = 2;
constexpr int32_t TEST_DARK_MODE_STATE_THREE = 3;
constexpr int32_t TEST_DARK_MODE_STATE_FOUR = 4;
constexpr int32_t TEST_DARK_MODE_STATE_FIVE = 5;
struct DarkModeState {
    int32_t settingMode = -1;
    int32_t settingStartTime = -1;
    int32_t settingEndTime = -1;
    int32_t settingSunsetTime = 1080;
    int32_t settingSunriseTime = 1860;
};
using DarkModeMode = DarkModeManager::DarkModeMode;
}

class DarkModeManagerInitializeTest : public Test {
protected:
    void TearDown() override
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        manager.settingDataObservers_.clear();
        manager.settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(INVALID_USER_ID);
        manager.settingDataObserversUserId_ = INVALID_USER_ID;
        manager.darkModeStates_.clear();
        manager.updateCallback_ = nullptr;
    }

    MOCK_METHOD(void, UpdateCallback1, (bool, int32_t), (const));
    MOCK_METHOD(void, UpdateCallback2, (bool, int32_t), (const));
};

class DarkModeManagerTest : public Test {
protected:
    void SetUp() override
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        EXPECT_EQ(manager.Initialize([this](const bool isDarkMode, const int32_t userId) {
            UpdateCallback(isDarkMode, userId);
            }), ERR_OK);
    }

    void TearDown() override
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        manager.settingDataObservers_.clear();
        manager.settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(INVALID_USER_ID);
        manager.settingDataObserversUserId_ = INVALID_USER_ID;
        manager.darkModeStates_.clear();
        manager.updateCallback_ = nullptr;
    }

    void LoadUserSettingDataInvalidDarkModeTest(
        const int32_t userId, const DarkModeState darkModeState, const DarkModeMode expectDarkMode) const
    {
        ExpectationSet expectSet;
        LoadUserSettingDataGetInt32ValueTest(userId, darkModeState, expectSet);
        EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);

        DarkModeManager& manager = DarkModeManager::GetInstance();
        bool isDarkMode = false;
        EXPECT_EQ(manager.LoadUserSettingData(userId, true, isDarkMode, false), ERR_INVALID_OPERATION);
        EXPECT_EQ(isDarkMode, false);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, expectDarkMode);
        EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, darkModeState.settingStartTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, darkModeState.settingEndTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunsetTime, darkModeState.settingSunsetTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunriseTime, darkModeState.settingSunriseTime);
    }

    void LoadUserSettingDataAllDayModeTest(const int32_t userId, const bool needUpdateCallback,
        const DarkModeState darkModeState, const DarkModeMode expectDarkMode) const
    {
        ExpectationSet expectSet;
        LoadUserSettingDataGetInt32ValueTest(userId, darkModeState, expectSet);

        bool expectIsDarkMode = expectDarkMode == DarkModeMode::DARK_MODE_ALWAYS_DARK;
        DarkModeManager& manager = DarkModeManager::GetInstance();
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, ClearTimerByUserId(userId)).Times(1).After(expectSet);
        if (needUpdateCallback) {
            expectSet += EXPECT_CALL(*this, UpdateCallback(expectIsDarkMode, userId)).Times(1).After(expectSet);
        } else {
            EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);
        }

        bool isDarkMode = !expectIsDarkMode;
        EXPECT_EQ(manager.LoadUserSettingData(userId, needUpdateCallback, isDarkMode, false), ERR_OK);
        EXPECT_EQ(isDarkMode, expectIsDarkMode);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, expectDarkMode);
        EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, darkModeState.settingStartTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, darkModeState.settingEndTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunsetTime, darkModeState.settingSunsetTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunriseTime, darkModeState.settingSunriseTime);
    }

    void LoadUserSettingDataCustomAutoModeSetTimerFailTest(
        const int32_t userId, const DarkModeState darkModeState, const DarkModeMode expectDarkMode) const
    {
        ExpectationSet expectSet;
        LoadUserSettingDataGetInt32ValueTest(userId, darkModeState, expectSet);

        DarkModeManager& manager = DarkModeManager::GetInstance();
        if (expectDarkMode == DarkModeMode::DARK_MODE_CUSTOM_AUTO) {
            expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(
                darkModeState.settingStartTime, darkModeState.settingEndTime, userId, _, _))
                .Times(1).After(expectSet).WillOnce(Return(TEST_ERROR));
        } else {
            expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(
                darkModeState.settingSunsetTime, darkModeState.settingSunriseTime, userId, _, _))
                .Times(1).After(expectSet).WillOnce(Return(TEST_ERROR));
        }
        
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, ClearTimerByUserId(userId)).Times(1).After(expectSet);
        EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);

        bool isDarkMode = false;
        EXPECT_EQ(manager.LoadUserSettingData(userId, true, isDarkMode, false), TEST_ERROR);
        EXPECT_EQ(isDarkMode, false);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, expectDarkMode);
        EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, darkModeState.settingStartTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, darkModeState.settingEndTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunsetTime, darkModeState.settingSunsetTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunriseTime, darkModeState.settingSunriseTime);
    }

    void LoadUserSettingDataCustomAutoModeTest(const int32_t userId, const bool needUpdateCallback,
        const DarkModeState darkModeState, const DarkModeMode expectDarkMode, const bool expectWithin) const
    {
        ExpectationSet expectSet;
        LoadUserSettingDataGetInt32ValueTest(userId, darkModeState, expectSet);

        DarkModeManager& manager = DarkModeManager::GetInstance();
        AlarmTimerManager& alarmTimerManagerStaticInstance = AlarmTimerManager::GetInstance();
        if (expectDarkMode == DarkModeMode::DARK_MODE_CUSTOM_AUTO) {
            expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(
                darkModeState.settingStartTime, darkModeState.settingEndTime, userId, _, _))
                .Times(1).After(expectSet).WillOnce(Return(ERR_OK));
            expectSet += EXPECT_CALL(alarmTimerManagerStaticInstance, MockIsWithinTimeInterval(
                darkModeState.settingStartTime, darkModeState.settingEndTime))
                .Times(1).After(expectSet).WillOnce(Return(expectWithin));
        } else {
            expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(
                darkModeState.settingSunsetTime, darkModeState.settingSunriseTime, userId, _, _))
                .Times(1).After(expectSet).WillOnce(Return(ERR_OK));
            expectSet += EXPECT_CALL(alarmTimerManagerStaticInstance, MockIsWithinTimeInterval(
                darkModeState.settingSunsetTime, darkModeState.settingSunriseTime))
                .Times(1).After(expectSet).WillOnce(Return(expectWithin));
        }

        if (needUpdateCallback) {
            expectSet += EXPECT_CALL(*this, UpdateCallback(expectWithin, userId)).Times(1).After(expectSet);
        } else {
            EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);
        }

        bool isDarkMode = !expectWithin;
        EXPECT_EQ(manager.LoadUserSettingData(userId, needUpdateCallback, isDarkMode, false), ERR_OK);
        EXPECT_EQ(isDarkMode, expectWithin);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, expectDarkMode);
        EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, darkModeState.settingStartTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, darkModeState.settingEndTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunsetTime, darkModeState.settingSunsetTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingSunriseTime, darkModeState.settingSunriseTime);
    }

    void NotifyDarkModeUpdateTest(const int32_t userId, const DarkModeMode origMode,
        const bool isDarkMode, const bool expectCallSetStringValue) const
    {
        SettingDataManager& dataManager = SettingDataManager::GetInstance();
        DarkModeMode newMode = isDarkMode ? DarkModeMode::DARK_MODE_ALWAYS_DARK : DarkModeMode::DARK_MODE_ALWAYS_LIGHT;
        if (expectCallSetStringValue) {
            EXPECT_CALL(dataManager, MockSetStringValue(SETTING_DARK_MODE_MODE, std::to_string(newMode), userId, true))
                .Times(1).WillOnce(Return(ERR_OK));
        } else {
            EXPECT_CALL(dataManager, MockSetStringValue(_, _, _, _)).Times(0);
        }

        DarkModeManager& manager = DarkModeManager::GetInstance();
        auto& state = manager.darkModeStates_[userId];
        state.settingMode = origMode;
        manager.NotifyDarkModeUpdate(userId, isDarkMode);
    }

    void OnSwitchUserTest(const int32_t userId, const int32_t origUserId, const bool registerObsFail) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        manager.settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(origUserId);
        manager.settingDataObserversUserId_ = origUserId;

        ExpectationSet expectSet;
        SettingDataManager& dataManager = SettingDataManager::GetInstance();
        expectSet += EXPECT_CALL(dataManager, IsInitialized()).Times(1).After(expectSet).WillOnce(Return(true));
        if (origUserId != INVALID_USER_ID) {
            expectSet += EXPECT_CALL(manager.alarmTimerManager_, ClearTimerByUserId(origUserId))
                .Times(1).After(expectSet);
            ExpectationSet input = expectSet;
            expectSet += EXPECT_CALL(dataManager, MockUnregisterObserver(SETTING_DARK_MODE_MODE, origUserId))
                .Times(1).After(input).WillOnce(Return(ERR_OK));
            expectSet += EXPECT_CALL(dataManager, MockUnregisterObserver(SETTING_DARK_MODE_START_TIME, origUserId))
                .Times(1).After(input).WillOnce(Return(ERR_OK));
            expectSet += EXPECT_CALL(dataManager, MockUnregisterObserver(SETTING_DARK_MODE_END_TIME, origUserId))
                .Times(1).After(input).WillOnce(Return(ERR_OK));
            expectSet += EXPECT_CALL(dataManager, MockUnregisterObserver(SETTING_DARK_MODE_SUN_SET, origUserId))
                .Times(1).After(input).WillOnce(Return(ERR_OK));
            expectSet += EXPECT_CALL(dataManager, MockUnregisterObserver(SETTING_DARK_MODE_SUN_RISE, origUserId))
                .Times(1).After(input).WillOnce(Return(ERR_OK));
        }

        ExpectationSet input = expectSet;
        expectSet += EXPECT_CALL(dataManager, MockRegisterObserver(SETTING_DARK_MODE_MODE, _, userId))
            .Times(1).After(input).WillOnce(Return(registerObsFail ? TEST_ERROR : ERR_OK));
        expectSet += EXPECT_CALL(dataManager, MockRegisterObserver(SETTING_DARK_MODE_START_TIME, _, userId))
            .Times(1).After(input).WillOnce(Return(ERR_OK));
        expectSet += EXPECT_CALL(dataManager, MockRegisterObserver(SETTING_DARK_MODE_END_TIME, _, userId))
            .Times(1).After(input).WillOnce(Return(ERR_OK));
        expectSet += EXPECT_CALL(dataManager, MockRegisterObserver(SETTING_DARK_MODE_SUN_SET, _, userId))
            .Times(1).After(input).WillOnce(Return(ERR_OK));
        expectSet += EXPECT_CALL(dataManager, MockRegisterObserver(SETTING_DARK_MODE_SUN_RISE, _, userId))
            .Times(1).After(input).WillOnce(Return(ERR_OK));

        EXPECT_EQ(manager.OnSwitchUser(userId), registerObsFail ? ERR_NO_INIT : ERR_OK);
        EXPECT_EQ(manager.settingDataObserversUserId_, userId);
    }

    void RestartTimerNoChangeTest(const int32_t userId, const DarkModeMode mode) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        manager.settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(userId);
        manager.settingDataObserversUserId_ = userId;
        manager.darkModeStates_[userId].settingMode = mode;
        EXPECT_EQ(manager.RestartTimer(), ERR_OK);
    }

    void RestartTimerTest(const int32_t userId, const int32_t startTime, const int32_t endTime,
        const bool expectWithin) const
    {
        ExpectationSet expectSet;
        DarkModeManager& manager = DarkModeManager::GetInstance();
        manager.settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(userId);
        manager.settingDataObserversUserId_ = userId;
        manager.darkModeStates_[userId].settingMode = DarkModeMode::DARK_MODE_CUSTOM_AUTO;
        manager.darkModeStates_[userId].settingStartTime = startTime;
        manager.darkModeStates_[userId].settingEndTime = endTime;

        AlarmTimerManager& alarmTimerManagerStaticInstance = AlarmTimerManager::GetInstance();
        expectSet += EXPECT_CALL(alarmTimerManagerStaticInstance, MockIsWithinTimeInterval(startTime, endTime))
            .Times(1).After(expectSet).WillOnce(Return(expectWithin));
        expectSet += EXPECT_CALL(*this, UpdateCallback(expectWithin, userId)).Times(1).After(expectSet);
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, RestartAllTimer())
            .Times(1).After(expectSet).WillOnce(Return(ERR_OK));
        EXPECT_EQ(manager.RestartTimer(), ERR_OK);
    }

    void ModeUpdateFuncFailTest(const int32_t userId, const DarkModeMode getDarkMode) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeMode = [getDarkMode](Unused, int32_t& value, Unused) {
            value = getDarkMode;
            return TEST_ERROR;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_MODE, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeMode));
        EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);
        updateFuncMap[SETTING_DARK_MODE_MODE](SETTING_DARK_MODE_MODE, userId);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, DarkModeMode::DARK_MODE_INVALID);
    }

    void ModeUpdateFuncAllDayTest(const int32_t userId, const DarkModeMode getDarkMode) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeMode = [getDarkMode](Unused, int32_t& value, Unused) {
            value = getDarkMode;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_MODE, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeMode));
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, ClearTimerByUserId(userId)).Times(1).After(expectSet);
        expectSet += EXPECT_CALL(*this, UpdateCallback(getDarkMode == DarkModeMode::DARK_MODE_ALWAYS_DARK, userId))
            .Times(1).After(expectSet);

        updateFuncMap[SETTING_DARK_MODE_MODE](SETTING_DARK_MODE_MODE, userId);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, getDarkMode);
    }

    void ModeUpdateFuncCustomAutoFailTest(const int32_t userId, const int32_t startTime, const int32_t endTime) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        manager.darkModeStates_[userId].settingStartTime = startTime;
        manager.darkModeStates_[userId].settingEndTime = endTime;

        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeMode = [](Unused, int32_t& value, Unused) {
            value = DarkModeMode::DARK_MODE_CUSTOM_AUTO;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_MODE, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeMode));
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(startTime, endTime, userId, _, _))
            .Times(1).After(expectSet).WillOnce(Return(TEST_ERROR));
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, ClearTimerByUserId(userId)).Times(1).After(expectSet);
        EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);
        updateFuncMap[SETTING_DARK_MODE_MODE](SETTING_DARK_MODE_MODE, userId);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    }

    void ModeUpdateFuncCustomAutoTest(const int32_t userId, const int32_t startTime, const int32_t endTime,
        const bool expectWithin) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        manager.darkModeStates_[userId].settingStartTime = startTime;
        manager.darkModeStates_[userId].settingEndTime = endTime;

        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeMode = [](Unused, int32_t& value, Unused) {
            value = DarkModeMode::DARK_MODE_CUSTOM_AUTO;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_MODE, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeMode));
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(startTime, endTime, userId, _, _))
            .Times(1).After(expectSet).WillOnce(Return(ERR_OK));
        AlarmTimerManager& alarmTimerManagerStaticInstance = AlarmTimerManager::GetInstance();
        expectSet += EXPECT_CALL(alarmTimerManagerStaticInstance, MockIsWithinTimeInterval(startTime, endTime))
            .Times(1).After(expectSet).WillOnce(Return(expectWithin));
        expectSet += EXPECT_CALL(*this, UpdateCallback(expectWithin, userId))
            .Times(1).After(expectSet);

        updateFuncMap[SETTING_DARK_MODE_MODE](SETTING_DARK_MODE_MODE, userId);
        EXPECT_EQ(manager.darkModeStates_[userId].settingMode, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    }

    void TimeUpdateFuncFailTest(const int32_t userId, const std::string& key,
        const int32_t expectValue, const int32_t code) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        manager.darkModeStates_[userId].settingMode = DarkModeMode::DARK_MODE_INVALID;

        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeTime = [expectValue, code](Unused, int32_t& value, Unused) {
            if (code == ERR_OK) {
                value = expectValue;
            }
            return code;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(key, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeTime));
        EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);
        updateFuncMap[key](key, userId);
        if (key == SETTING_DARK_MODE_START_TIME) {
            EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, code == ERR_OK ? expectValue : -1);
        } else {
            EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, code == ERR_OK ? expectValue : -1);
        }
    }

    void TimeUpdateFuncAllDayTest(const int32_t userId, DarkModeMode mode, const std::string& key,
        const int32_t expectValue) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        manager.darkModeStates_[userId].settingMode = mode;

        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeTime = [expectValue](Unused, int32_t& value, Unused) {
            value = expectValue;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(key, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeTime));
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, ClearTimerByUserId(userId)).Times(1).After(expectSet);
        expectSet += EXPECT_CALL(*this, UpdateCallback(mode == DarkModeMode::DARK_MODE_ALWAYS_DARK, userId))
            .Times(1).After(expectSet);
        updateFuncMap[key](key, userId);
        if (key == SETTING_DARK_MODE_START_TIME) {
            EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, expectValue);
        } else {
            EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, expectValue);
        }
    }

    void TimeUpdateFuncCustomAutoFailTest(const int32_t userId, const std::string& key, const int32_t expectValue,
        const int32_t otherValue) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        manager.darkModeStates_[userId].settingMode = DarkModeMode::DARK_MODE_CUSTOM_AUTO;

        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeTime = [expectValue](Unused, int32_t& value, Unused) {
            value = expectValue;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(key, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeTime));
        int32_t startTime;
        int32_t endTime;
        if (key == SETTING_DARK_MODE_START_TIME) {
            startTime = expectValue;
            endTime = otherValue;
            manager.darkModeStates_[userId].settingEndTime = otherValue;
        } else {
            startTime = otherValue;
            endTime = expectValue;
            manager.darkModeStates_[userId].settingStartTime = otherValue;
        }

        expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(startTime, endTime, userId, _, _))
            .Times(1).After(expectSet).WillOnce(Return(TEST_ERROR));
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, ClearTimerByUserId(userId)).Times(1).After(expectSet);
        EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);
        updateFuncMap[key](key, userId);
        EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, startTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, endTime);
    }

    void TimeUpdateFuncCustomAutoTest(const int32_t userId, const std::string& key, const int32_t expectValue,
        const int32_t otherValue, const bool expectWithin) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        manager.darkModeStates_[userId].settingMode = DarkModeMode::DARK_MODE_CUSTOM_AUTO;

        ExpectationSet expectSet;
        std::map<std::string, std::function<void(const std::string&, int32_t)>> updateFuncMap;
        GetUpdateFuncMap(userId, expectSet, updateFuncMap);

        auto checkSettingDarkModeTime = [expectValue](Unused, int32_t& value, Unused) {
            value = expectValue;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(key, _, userId))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSettingDarkModeTime));
        int32_t startTime;
        int32_t endTime;
        if (key == SETTING_DARK_MODE_START_TIME) {
            startTime = expectValue;
            endTime = otherValue;
            manager.darkModeStates_[userId].settingEndTime = otherValue;
        } else {
            startTime = otherValue;
            endTime = expectValue;
            manager.darkModeStates_[userId].settingStartTime = otherValue;
        }

        expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(startTime, endTime, userId, _, _))
            .Times(1).After(expectSet).WillOnce(Return(ERR_OK));
        AlarmTimerManager& alarmTimerManagerStaticInstance = AlarmTimerManager::GetInstance();
        expectSet += EXPECT_CALL(alarmTimerManagerStaticInstance, MockIsWithinTimeInterval(startTime, endTime))
            .Times(1).After(expectSet).WillOnce(Return(expectWithin));
        expectSet += EXPECT_CALL(*this, UpdateCallback(expectWithin, userId))
            .Times(1).After(expectSet);
        updateFuncMap[key](key, userId);
        EXPECT_EQ(manager.darkModeStates_[userId].settingStartTime, startTime);
        EXPECT_EQ(manager.darkModeStates_[userId].settingEndTime, endTime);
    }

    void TimerCallbackFailTest(const int32_t userId, const int32_t startTime, const int32_t endTime,
        const DarkModeMode stateMode, const int32_t stateStartTime, const int32_t stateEndTine) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        manager.darkModeStates_[userId].settingMode = stateMode;
        manager.darkModeStates_[userId].settingStartTime = stateStartTime;
        manager.darkModeStates_[userId].settingEndTime = stateEndTine;

        ExpectationSet expectSet;
        std::function<void()> startCallbackFunc;
        std::function<void()> endCallbackFunc;
        auto checkSetScheduleTime = [&startCallbackFunc, &endCallbackFunc](Unused, Unused, Unused,
            const std::function<void()>& startCallback, const std::function<void()>& endCallback) {
            startCallbackFunc = startCallback;
            endCallbackFunc = endCallback;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(startTime, endTime, userId, _, _))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSetScheduleTime));
        EXPECT_EQ(manager.CreateOrUpdateTimers(startTime, endTime, userId), ERR_OK);
        EXPECT_NE(startCallbackFunc, nullptr);
        EXPECT_NE(endCallbackFunc, nullptr);
        EXPECT_CALL(*this, UpdateCallback(_, _)).Times(0);
        startCallbackFunc();
        endCallbackFunc();
    }

    void TimerCallbackTest(const int32_t userId, const int32_t startTime, const int32_t endTime) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        manager.darkModeStates_[userId].settingMode = DarkModeMode::DARK_MODE_CUSTOM_AUTO;
        manager.darkModeStates_[userId].settingStartTime = startTime;
        manager.darkModeStates_[userId].settingEndTime = endTime;

        ExpectationSet expectSet;
        std::function<void()> startCallbackFunc;
        std::function<void()> endCallbackFunc;
        auto checkSetScheduleTime = [&startCallbackFunc, &endCallbackFunc](Unused, Unused, Unused,
            const std::function<void()>& startCallback, const std::function<void()>& endCallback) {
            startCallbackFunc = startCallback;
            endCallbackFunc = endCallback;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(manager.alarmTimerManager_, SetScheduleTime(startTime, endTime, userId, _, _))
            .Times(1).After(expectSet).WillOnce(Invoke(checkSetScheduleTime));
        EXPECT_EQ(manager.CreateOrUpdateTimers(startTime, endTime, userId), ERR_OK);
        EXPECT_NE(startCallbackFunc, nullptr);
        EXPECT_NE(endCallbackFunc, nullptr);
    }

    MOCK_METHOD(void, UpdateCallback, (bool, int32_t), (const));

private:
    void LoadUserSettingDataGetInt32ValueTest(
        const int32_t userId, const DarkModeState darkModeState, ExpectationSet& expectSet) const
    {
        ExpectationSet input = expectSet;
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();
        auto checkSettingDarkModeMode = [darkModeState](Unused, int32_t& value, Unused) {
            value = darkModeState.settingMode;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_MODE, _, userId))
            .Times(1).After(input).WillOnce(Invoke(checkSettingDarkModeMode));
        auto checkSettingDarkModeStartTime = [darkModeState](Unused, int32_t& value, Unused) {
            value = darkModeState.settingStartTime;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_START_TIME, _, userId))
            .Times(1).After(input).WillOnce(Invoke(checkSettingDarkModeStartTime));
        auto checkSettingDarkModeEndTime = [darkModeState](Unused, int32_t& value, Unused) {
            value = darkModeState.settingEndTime;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_END_TIME, _, userId))
            .Times(1).After(input).WillOnce(Invoke(checkSettingDarkModeEndTime));
        auto checkSettingDarkModeSunsetTime = [darkModeState](Unused, int32_t& value, Unused) {
            value = darkModeState.settingSunsetTime;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_SUN_SET, _, userId))
            .Times(1).After(input).WillOnce(Invoke(checkSettingDarkModeSunsetTime));
        auto checkSettingDarkModeSunriseTime = [darkModeState](Unused, int32_t& value, Unused) {
            value = darkModeState.settingSunriseTime;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockGetInt32ValueStrictly(SETTING_DARK_MODE_SUN_RISE, _, userId))
            .Times(1).After(input).WillOnce(Invoke(checkSettingDarkModeSunriseTime));
    }

    void GetUpdateFuncMap(const int32_t userId, ExpectationSet& expectSet,
        std::map<std::string, std::function<void(const std::string&, int32_t)>>& updateFuncMap) const
    {
        DarkModeManager& manager = DarkModeManager::GetInstance();
        SettingDataManager& settingDataManager = SettingDataManager::GetInstance();

        manager.settingDataObserversContext_ = AccountContextHelper::CreateBaseContext(INVALID_USER_ID);
        manager.settingDataObserversUserId_ = INVALID_USER_ID;
        expectSet += EXPECT_CALL(settingDataManager, IsInitialized()).Times(1).After(expectSet).WillOnce(Return(true));
        auto checkRegisterObserver = [&updateFuncMap](const std::string& key,
            const std::function<void(const std::string&, int32_t)>& updateFunc, Unused) {
            updateFuncMap[key] = updateFunc;
            return ERR_OK;
        };
        expectSet += EXPECT_CALL(settingDataManager, MockRegisterObserver(SETTING_DARK_MODE_MODE, _, userId))
            .Times(1).WillOnce(Invoke(checkRegisterObserver));
        expectSet += EXPECT_CALL(settingDataManager, MockRegisterObserver(SETTING_DARK_MODE_START_TIME, _, userId))
            .Times(1).WillOnce(Invoke(checkRegisterObserver));
        expectSet += EXPECT_CALL(settingDataManager, MockRegisterObserver(SETTING_DARK_MODE_END_TIME, _, userId))
            .Times(1).WillOnce(Invoke(checkRegisterObserver));
        expectSet += EXPECT_CALL(settingDataManager, MockRegisterObserver(SETTING_DARK_MODE_SUN_SET, _, userId))
            .Times(1).WillOnce(Invoke(checkRegisterObserver));
        expectSet += EXPECT_CALL(settingDataManager, MockRegisterObserver(SETTING_DARK_MODE_SUN_RISE, _, userId))
            .Times(1).WillOnce(Invoke(checkRegisterObserver));
        EXPECT_EQ(manager.OnSwitchUser(userId), ERR_OK);
        EXPECT_EQ(manager.settingDataObserversUserId_, userId);
        EXPECT_EQ(updateFuncMap.size(), SETTING_NUM);
        EXPECT_NE(updateFuncMap[SETTING_DARK_MODE_MODE], nullptr);
        EXPECT_NE(updateFuncMap[SETTING_DARK_MODE_START_TIME], nullptr);
        EXPECT_NE(updateFuncMap[SETTING_DARK_MODE_END_TIME], nullptr);
        EXPECT_NE(updateFuncMap[SETTING_DARK_MODE_SUN_SET], nullptr);
        EXPECT_NE(updateFuncMap[SETTING_DARK_MODE_SUN_RISE], nullptr);
    }
};

HWTEST_F(DarkModeManagerInitializeTest, InitializeTest_0100, TestSize.Level1)
{
    DarkModeManager& manager = DarkModeManager::GetInstance();
    EXPECT_EQ(manager.settingDataObservers_.size(), 0);
    EXPECT_EQ(manager.settingDataObserversUserId_, INVALID_USER_ID);
    EXPECT_EQ(manager.darkModeStates_.size(), 0);
    EXPECT_EQ(manager.updateCallback_, nullptr);

    EXPECT_EQ(manager.Initialize([this](const bool isDarkMode, const int32_t userId) {
        UpdateCallback1(isDarkMode, userId);
        }), ERR_OK);
    EXPECT_EQ(manager.settingDataObservers_.size(), SETTING_NUM);
    EXPECT_EQ(manager.settingDataObserversUserId_, INVALID_USER_ID);
    EXPECT_EQ(manager.darkModeStates_.size(), 0);
    EXPECT_NE(manager.updateCallback_, nullptr);

    EXPECT_CALL(*this, UpdateCallback1(true, INVALID_USER_ID)).Times(1);
    EXPECT_CALL(*this, UpdateCallback1(false, TEST_USER1)).Times(1);
    EXPECT_CALL(*this, UpdateCallback1(true, TEST_USER100)).Times(1);
    manager.updateCallback_(true, INVALID_USER_ID);
    manager.updateCallback_(false, TEST_USER1);
    manager.updateCallback_(true, TEST_USER100);

    EXPECT_EQ(manager.Initialize([this](const bool isDarkMode, const int32_t userId) {
        UpdateCallback2(isDarkMode, userId);
        }), ERR_OK);
    EXPECT_EQ(manager.settingDataObservers_.size(), SETTING_NUM);
    EXPECT_EQ(manager.settingDataObserversUserId_, INVALID_USER_ID);
    EXPECT_EQ(manager.darkModeStates_.size(), 0);
    EXPECT_NE(manager.updateCallback_, nullptr);

    EXPECT_CALL(*this, UpdateCallback2(true, INVALID_USER_ID)).Times(1);
    EXPECT_CALL(*this, UpdateCallback2(false, TEST_USER1)).Times(1);
    EXPECT_CALL(*this, UpdateCallback2(true, TEST_USER100)).Times(1);
    manager.updateCallback_(true, INVALID_USER_ID);
    manager.updateCallback_(false, TEST_USER1);
    manager.updateCallback_(true, TEST_USER100);
}

HWTEST_F(DarkModeManagerTest, LoadUserSettingData_0100, TestSize.Level1)
{
    DarkModeState darkModeState;
    darkModeState.settingMode = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ONE;
    LoadUserSettingDataInvalidDarkModeTest(INVALID_USER_ID, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER1, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER100, darkModeState, DarkModeMode::DARK_MODE_INVALID);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ZERO;
    LoadUserSettingDataInvalidDarkModeTest(INVALID_USER_ID, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER1, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER100, darkModeState, DarkModeMode::DARK_MODE_INVALID);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_FOUR;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    LoadUserSettingDataInvalidDarkModeTest(INVALID_USER_ID, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER1, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER100, darkModeState, DarkModeMode::DARK_MODE_INVALID);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_FIVE;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    LoadUserSettingDataInvalidDarkModeTest(INVALID_USER_ID, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER1, darkModeState, DarkModeMode::DARK_MODE_INVALID);
    LoadUserSettingDataInvalidDarkModeTest(TEST_USER100, darkModeState, DarkModeMode::DARK_MODE_INVALID);
}

HWTEST_F(DarkModeManagerTest, LoadUserSettingData_0200, TestSize.Level1)
{
    DarkModeState darkModeState;
    darkModeState.settingMode = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ONE;
    LoadUserSettingDataAllDayModeTest(INVALID_USER_ID, true, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    LoadUserSettingDataAllDayModeTest(TEST_USER1, true, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    LoadUserSettingDataAllDayModeTest(TEST_USER100, true, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ZERO;
    LoadUserSettingDataAllDayModeTest(INVALID_USER_ID, false, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    LoadUserSettingDataAllDayModeTest(TEST_USER1, false, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    LoadUserSettingDataAllDayModeTest(TEST_USER100, false, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    LoadUserSettingDataAllDayModeTest(INVALID_USER_ID, true, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    LoadUserSettingDataAllDayModeTest(TEST_USER1, true, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    LoadUserSettingDataAllDayModeTest(TEST_USER100, true, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_DARK);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    LoadUserSettingDataAllDayModeTest(INVALID_USER_ID, false, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    LoadUserSettingDataAllDayModeTest(TEST_USER1, false, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    LoadUserSettingDataAllDayModeTest(TEST_USER100, false, darkModeState, DarkModeMode::DARK_MODE_ALWAYS_DARK);
}

HWTEST_F(DarkModeManagerTest, LoadUserSettingData_0300, TestSize.Level1)
{
    DarkModeState darkModeState;
    darkModeState.settingMode = TEST_DARK_MODE_STATE_TWO;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ONE;
    LoadUserSettingDataCustomAutoModeSetTimerFailTest(
        INVALID_USER_ID, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    darkModeState.settingMode = TEST_DARK_MODE_STATE_THREE;
    LoadUserSettingDataCustomAutoModeSetTimerFailTest(
        INVALID_USER_ID, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_TWO;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ZERO;
    LoadUserSettingDataCustomAutoModeSetTimerFailTest(
        TEST_USER1, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    darkModeState.settingMode = TEST_DARK_MODE_STATE_THREE;
    LoadUserSettingDataCustomAutoModeSetTimerFailTest(
        TEST_USER1, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET);

    darkModeState.settingMode = TEST_DARK_MODE_STATE_TWO;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    LoadUserSettingDataCustomAutoModeSetTimerFailTest(
        TEST_USER100, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    darkModeState.settingMode = TEST_DARK_MODE_STATE_THREE;
    LoadUserSettingDataCustomAutoModeSetTimerFailTest(
        TEST_USER100, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET);
}

HWTEST_F(DarkModeManagerTest, LoadUserSettingData_0400, TestSize.Level1)
{
    DarkModeState darkModeState;
    darkModeState.settingMode = TEST_DARK_MODE_STATE_TWO;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ONE;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, true, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, true, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, true, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, true);

    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ZERO;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, false, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, false, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, false, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, true);

    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, true, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, true, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, true, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, false);

    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, false, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, false, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, false, darkModeState, DarkModeMode::DARK_MODE_CUSTOM_AUTO, false);
}

HWTEST_F(DarkModeManagerTest, LoadUserSettingData_0500, TestSize.Level1)
{
    DarkModeState darkModeState;
    darkModeState.settingMode = TEST_DARK_MODE_STATE_THREE;
    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ONE;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, true);

    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_ZERO;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, false, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, false, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, true);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, false, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, true);

    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ZERO;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_ONE;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, false);

    darkModeState.settingStartTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingEndTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    darkModeState.settingSunsetTime = TEST_DARK_MODE_STATE_ONE;
    darkModeState.settingSunriseTime = TEST_DARK_MODE_STATE_NEGATIVE_TWO;
    LoadUserSettingDataCustomAutoModeTest(
        INVALID_USER_ID, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER1, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, false);
    LoadUserSettingDataCustomAutoModeTest(
        TEST_USER100, true, darkModeState, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, false);
}

HWTEST_F(DarkModeManagerTest, NotifyDarkModeUpdate_0100, TestSize.Level1)
{
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_INVALID, true, true);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_INVALID, false, true);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, true, true);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, false, false);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_ALWAYS_DARK, true, false);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_ALWAYS_DARK, false, true);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_CUSTOM_AUTO, true, false);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_CUSTOM_AUTO, false, false);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, true, false);
    NotifyDarkModeUpdateTest(INVALID_USER_ID, DarkModeMode::DARK_MODE_SUNRISE_SUNSET, false, false);
}

HWTEST_F(DarkModeManagerTest, OnSwitchUser_0100, TestSize.Level1)
{
    ExpectationSet expectSet;
    SettingDataManager& dataManager = SettingDataManager::GetInstance();
    expectSet += EXPECT_CALL(dataManager, IsInitialized()).Times(1).After(expectSet).WillOnce(Return(false));
    expectSet += EXPECT_CALL(dataManager, Initialize()).Times(1).After(expectSet).WillOnce(Return(TEST_ERROR));
    DarkModeManager& manager = DarkModeManager::GetInstance();
    EXPECT_EQ(manager.OnSwitchUser(INVALID_USER_ID), ERR_NO_INIT);
}

HWTEST_F(DarkModeManagerTest, OnSwitchUser_0200, TestSize.Level1)
{
    ExpectationSet expectSet;
    SettingDataManager& dataManager = SettingDataManager::GetInstance();
    expectSet += EXPECT_CALL(dataManager, IsInitialized()).Times(1).After(expectSet).WillOnce(Return(false));
    expectSet += EXPECT_CALL(dataManager, Initialize()).Times(1).After(expectSet).WillOnce(Return(ERR_OK));
    expectSet += EXPECT_CALL(dataManager, IsInitialized()).Times(1).After(expectSet).WillOnce(Return(false));
    DarkModeManager& manager = DarkModeManager::GetInstance();
    EXPECT_EQ(manager.OnSwitchUser(INVALID_USER_ID), ERR_NO_INIT);
}

HWTEST_F(DarkModeManagerTest, OnSwitchUser_0300, TestSize.Level1)
{
    ExpectationSet expectSet;
    SettingDataManager& dataManager = SettingDataManager::GetInstance();
    expectSet += EXPECT_CALL(dataManager, IsInitialized()).Times(1).After(expectSet).WillOnce(Return(true));
    DarkModeManager& manager = DarkModeManager::GetInstance();
    EXPECT_EQ(manager.OnSwitchUser(INVALID_USER_ID), ERR_INVALID_OPERATION);
}

HWTEST_F(DarkModeManagerTest, OnSwitchUser_0400, TestSize.Level1)
{
    OnSwitchUserTest(TEST_USER1, TEST_USER1, true);
    OnSwitchUserTest(TEST_USER100, TEST_USER100, true);
    OnSwitchUserTest(TEST_USER101, TEST_USER101, true);

    OnSwitchUserTest(TEST_USER1, TEST_USER1, false);
    OnSwitchUserTest(TEST_USER100, TEST_USER100, false);
    OnSwitchUserTest(TEST_USER101, TEST_USER101, false);
}

HWTEST_F(DarkModeManagerTest, OnSwitchUser_0500, TestSize.Level1)
{
    OnSwitchUserTest(TEST_USER1, INVALID_USER_ID, false);
    OnSwitchUserTest(TEST_USER1, TEST_USER100, false);
    OnSwitchUserTest(TEST_USER1, TEST_USER101, false);
    OnSwitchUserTest(TEST_USER100, INVALID_USER_ID, false);
    OnSwitchUserTest(TEST_USER100, TEST_USER1, false);
    OnSwitchUserTest(TEST_USER100, TEST_USER101, false);
    OnSwitchUserTest(TEST_USER101, INVALID_USER_ID, false);
    OnSwitchUserTest(TEST_USER101, TEST_USER1, false);
    OnSwitchUserTest(TEST_USER101, TEST_USER100, false);

    OnSwitchUserTest(TEST_USER1, INVALID_USER_ID, true);
    OnSwitchUserTest(TEST_USER1, TEST_USER100, true);
    OnSwitchUserTest(TEST_USER1, TEST_USER101, true);
    OnSwitchUserTest(TEST_USER100, INVALID_USER_ID, true);
    OnSwitchUserTest(TEST_USER100, TEST_USER1, true);
    OnSwitchUserTest(TEST_USER100, TEST_USER101, true);
    OnSwitchUserTest(TEST_USER101, INVALID_USER_ID, true);
    OnSwitchUserTest(TEST_USER101, TEST_USER1, true);
    OnSwitchUserTest(TEST_USER101, TEST_USER100, true);
}

HWTEST_F(DarkModeManagerTest, RestartTimer_0100, TestSize.Level1)
{
    RestartTimerNoChangeTest(TEST_USER1, DarkModeMode::DARK_MODE_INVALID);
    RestartTimerNoChangeTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    RestartTimerNoChangeTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    RestartTimerNoChangeTest(TEST_USER100, DarkModeMode::DARK_MODE_INVALID);
    RestartTimerNoChangeTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    RestartTimerNoChangeTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    RestartTimerNoChangeTest(TEST_USER101, DarkModeMode::DARK_MODE_INVALID);
    RestartTimerNoChangeTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    RestartTimerNoChangeTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_DARK);
}

HWTEST_F(DarkModeManagerTest, RestartTimer_0200, TestSize.Level1)
{
    RestartTimerTest(TEST_USER1, -1, 4, true);
    RestartTimerTest(TEST_USER1, 0, 3, false);
    RestartTimerTest(TEST_USER100, 1, 2, true);
    RestartTimerTest(TEST_USER100, 2, 1, false);
    RestartTimerTest(TEST_USER101, 3, 0, true);
    RestartTimerTest(TEST_USER101, 4, -1, false);
}

HWTEST_F(DarkModeManagerTest, ModeUpdateFunc_0100, TestSize.Level1)
{
    ModeUpdateFuncFailTest(TEST_USER1, DarkModeMode::DARK_MODE_INVALID);
    ModeUpdateFuncFailTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    ModeUpdateFuncFailTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    ModeUpdateFuncFailTest(TEST_USER1, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    ModeUpdateFuncFailTest(TEST_USER1, DarkModeMode::DARK_MODE_SUNRISE_SUNSET);
    ModeUpdateFuncFailTest(TEST_USER1, DarkModeMode::DARK_MODE_SIZE);
    ModeUpdateFuncFailTest(TEST_USER100, DarkModeMode::DARK_MODE_INVALID);
    ModeUpdateFuncFailTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    ModeUpdateFuncFailTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    ModeUpdateFuncFailTest(TEST_USER100, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    ModeUpdateFuncFailTest(TEST_USER100, DarkModeMode::DARK_MODE_SUNRISE_SUNSET);
    ModeUpdateFuncFailTest(TEST_USER100, DarkModeMode::DARK_MODE_SIZE);
    ModeUpdateFuncFailTest(TEST_USER101, DarkModeMode::DARK_MODE_INVALID);
    ModeUpdateFuncFailTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    ModeUpdateFuncFailTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    ModeUpdateFuncFailTest(TEST_USER101, DarkModeMode::DARK_MODE_CUSTOM_AUTO);
    ModeUpdateFuncFailTest(TEST_USER101, DarkModeMode::DARK_MODE_SUNRISE_SUNSET);
    ModeUpdateFuncFailTest(TEST_USER101, DarkModeMode::DARK_MODE_SIZE);
}

HWTEST_F(DarkModeManagerTest, ModeUpdateFunc_0200, TestSize.Level1)
{
    ModeUpdateFuncAllDayTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    ModeUpdateFuncAllDayTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    ModeUpdateFuncAllDayTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    ModeUpdateFuncAllDayTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
    ModeUpdateFuncAllDayTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_DARK);
    ModeUpdateFuncAllDayTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_LIGHT);
}

HWTEST_F(DarkModeManagerTest, ModeUpdateFunc_0300, TestSize.Level1)
{
    ModeUpdateFuncCustomAutoFailTest(TEST_USER1, 0, 5);
    ModeUpdateFuncCustomAutoFailTest(TEST_USER1, 1, 4);
    ModeUpdateFuncCustomAutoFailTest(TEST_USER100, 2, 3);
    ModeUpdateFuncCustomAutoFailTest(TEST_USER100, 3, 2);
    ModeUpdateFuncCustomAutoFailTest(TEST_USER101, 4, 1);
    ModeUpdateFuncCustomAutoFailTest(TEST_USER101, 5, 0);
}

HWTEST_F(DarkModeManagerTest, ModeUpdateFunc_0400, TestSize.Level1)
{
    ModeUpdateFuncCustomAutoTest(TEST_USER1, 0, 5, true);
    ModeUpdateFuncCustomAutoTest(TEST_USER1, 1, 4, false);
    ModeUpdateFuncCustomAutoTest(TEST_USER100, 2, 3, true);
    ModeUpdateFuncCustomAutoTest(TEST_USER100, 3, 2, false);
    ModeUpdateFuncCustomAutoTest(TEST_USER101, 4, 1, true);
    ModeUpdateFuncCustomAutoTest(TEST_USER101, 5, 0, false);
}

HWTEST_F(DarkModeManagerTest, StartTimeUpdateFunc_0100, TestSize.Level1)
{
    TimeUpdateFuncFailTest(TEST_USER1, SETTING_DARK_MODE_START_TIME, -1, ERR_OK);
    TimeUpdateFuncFailTest(TEST_USER1, SETTING_DARK_MODE_END_TIME, 0, TEST_ERROR);
    TimeUpdateFuncFailTest(TEST_USER100, SETTING_DARK_MODE_START_TIME, 1, ERR_OK);
    TimeUpdateFuncFailTest(TEST_USER100, SETTING_DARK_MODE_END_TIME, 2, TEST_ERROR);
    TimeUpdateFuncFailTest(TEST_USER101, SETTING_DARK_MODE_START_TIME, 3, ERR_OK);
    TimeUpdateFuncFailTest(TEST_USER101, SETTING_DARK_MODE_END_TIME, 4, TEST_ERROR);
}

HWTEST_F(DarkModeManagerTest, StartTimeUpdateFunc_0200, TestSize.Level1)
{
    TimeUpdateFuncAllDayTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_DARK, SETTING_DARK_MODE_START_TIME, -1);
    TimeUpdateFuncAllDayTest(TEST_USER1, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, SETTING_DARK_MODE_END_TIME, 0);
    TimeUpdateFuncAllDayTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_DARK, SETTING_DARK_MODE_START_TIME, 1);
    TimeUpdateFuncAllDayTest(TEST_USER100, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, SETTING_DARK_MODE_END_TIME, 2);
    TimeUpdateFuncAllDayTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_DARK, SETTING_DARK_MODE_START_TIME, 3);
    TimeUpdateFuncAllDayTest(TEST_USER101, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, SETTING_DARK_MODE_END_TIME, 4);
}

HWTEST_F(DarkModeManagerTest, StartTimeUpdateFunc_0300, TestSize.Level1)
{
    TimeUpdateFuncCustomAutoFailTest(TEST_USER1, SETTING_DARK_MODE_START_TIME, -1, 4);
    TimeUpdateFuncCustomAutoFailTest(TEST_USER1, SETTING_DARK_MODE_END_TIME, 0, 3);
    TimeUpdateFuncCustomAutoFailTest(TEST_USER100, SETTING_DARK_MODE_START_TIME, 1, 2);
    TimeUpdateFuncCustomAutoFailTest(TEST_USER100, SETTING_DARK_MODE_END_TIME, 2, 1);
    TimeUpdateFuncCustomAutoFailTest(TEST_USER101, SETTING_DARK_MODE_START_TIME, 3, 0);
    TimeUpdateFuncCustomAutoFailTest(TEST_USER101, SETTING_DARK_MODE_END_TIME, 4, -1);
}

HWTEST_F(DarkModeManagerTest, StartTimeUpdateFunc_0400, TestSize.Level1)
{
    TimeUpdateFuncCustomAutoTest(TEST_USER1, SETTING_DARK_MODE_START_TIME, -1, 4, true);
    TimeUpdateFuncCustomAutoTest(TEST_USER1, SETTING_DARK_MODE_END_TIME, 0, 3, true);
    TimeUpdateFuncCustomAutoTest(TEST_USER100, SETTING_DARK_MODE_START_TIME, 1, 2, true);
    TimeUpdateFuncCustomAutoTest(TEST_USER100, SETTING_DARK_MODE_END_TIME, 2, 1, false);
    TimeUpdateFuncCustomAutoTest(TEST_USER101, SETTING_DARK_MODE_START_TIME, 3, 0, false);
    TimeUpdateFuncCustomAutoTest(TEST_USER101, SETTING_DARK_MODE_END_TIME, 4, -1, false);
}

HWTEST_F(DarkModeManagerTest, TimerCallback_0100, TestSize.Level1)
{
    TimerCallbackFailTest(TEST_USER1, -1, 7, DarkModeMode::DARK_MODE_INVALID, -1, 7);
    TimerCallbackFailTest(TEST_USER100, 0, 6, DarkModeMode::DARK_MODE_INVALID, 0, 6);
    TimerCallbackFailTest(TEST_USER101, 1, 5, DarkModeMode::DARK_MODE_INVALID, 1, 5);
    TimerCallbackFailTest(TEST_USER1, 2, 4, DarkModeMode::DARK_MODE_ALWAYS_DARK, 2, 4);
    TimerCallbackFailTest(TEST_USER100, 3, 3, DarkModeMode::DARK_MODE_ALWAYS_DARK, 3, 3);
    TimerCallbackFailTest(TEST_USER101, 4, 2, DarkModeMode::DARK_MODE_ALWAYS_DARK, 4, 2);
    TimerCallbackFailTest(TEST_USER1, 5, 1, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, 5, 1);
    TimerCallbackFailTest(TEST_USER100, 6, 0, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, 6, 0);
    TimerCallbackFailTest(TEST_USER101, 7, -1, DarkModeMode::DARK_MODE_ALWAYS_LIGHT, 7, -1);

    TimerCallbackFailTest(TEST_USER1, -1, 1, DarkModeMode::DARK_MODE_CUSTOM_AUTO, 0, 1);
    TimerCallbackFailTest(TEST_USER100, -1, 1, DarkModeMode::DARK_MODE_CUSTOM_AUTO, 0, 1);
    TimerCallbackFailTest(TEST_USER101, -1, 1, DarkModeMode::DARK_MODE_CUSTOM_AUTO, 0, 1);
    TimerCallbackFailTest(TEST_USER1, -1, 1, DarkModeMode::DARK_MODE_CUSTOM_AUTO, -1, 0);
    TimerCallbackFailTest(TEST_USER100, -1, 1, DarkModeMode::DARK_MODE_CUSTOM_AUTO, -1, 0);
    TimerCallbackFailTest(TEST_USER101, -1, 1, DarkModeMode::DARK_MODE_CUSTOM_AUTO, -1, 0);
}

HWTEST_F(DarkModeManagerTest, TimerCallback_0200, TestSize.Level1)
{
    TimerCallbackTest(TEST_USER1, -1, 1);
    TimerCallbackTest(TEST_USER100, 0, 0);
    TimerCallbackTest(TEST_USER101, 1, -1);
}

HWTEST_F(DarkModeManagerTest, CheckCurrentTimeInDarkInterval_0100, TestSize.Level1)
{
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 7.5 * 60 * 60), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 9 * 60 * 60), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 8 * 60 * 60), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 8 * 60 * 60 - 4), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 8 * 60 * 60 - 5), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 8 * 60 * 60 - 6), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 8 * 60 * 60 + 1), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 7 * 60 * 60), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 7 * 60 * 60 + 1), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 7 * 60 * 60 - 1), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 7 * 60 * 60 - 4), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 8 * 60, 7 * 60 * 60 - 5), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 23 * 60 * 60), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 10 * 60 * 60), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 22 * 60 * 60), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 22 * 60 * 60 + 1), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 22 * 60 * 60 - 1), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 22 * 60 * 60 - 4), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 22 * 60 * 60 - 5), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 7 * 60 * 60), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 7 * 60 * 60 + 1), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 7 * 60 * 60 - 1), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 7 * 60 * 60 - 4), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 7 * 60 * 60 - 5), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(22 * 60, (7 + 24) * 60, 7 * 60 * 60 - 6), true);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 0, 0), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 9, 0), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(9, 0, 0), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 0, 1), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 9, 1), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(9, 0, 1), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 0, 4), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 9, 4), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(9, 0, 4), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 0, 6), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(0, 9, 6), true);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(9, 0, 6), false);

    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 24 * 60, 24 * 60 * 60), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 24 * 60, 24 * 60 * 60 - 1), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 24 * 60, 24 * 60 * 60 - 4), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 24 * 60, 24 * 60 * 60 - 5), false);
    EXPECT_EQ(DarkModeManager::CheckCurrentTimeInDarkInterval(7 * 60, 24 * 60, 24 * 60 * 60 - 6), true);
}
} // namespace OHOS::ArkUi::UiAppearance
