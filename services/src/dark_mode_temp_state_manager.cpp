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

#include "dark_mode_temp_state_manager.h"

#include <ctime>
#include <sys/time.h>
#include <cinttypes>
#include "dark_mode_manager.h"
#include "parameter_wrap.h"
#include "ui_appearance_log.h"

namespace OHOS::ArkUi::UiAppearance {
namespace {
static const std::string TEMPORARY_COLOR_MODE_PARAM_STRING = "persist.uiAppearance.dark_mode_temp_state_flag.";
static const std::string TEMPORARY_STATE_START_TIME_PARAM_STRING =
    "persist.uiAppearance.dark_mode_temp_state_start_time.";
static const std::string TEMPORARY_STATE_END_TIME_PARAM_STRING = "persist.uiAppearance.dark_mode_temp_state_end_time.";
static const std::string TEMPORARY_COLOR_MODE_TEMPORARY_STRING = "1";
static const std::string TEMPORARY_COLOR_MODE_NORMAL_STRING = "0";
constexpr int32_t MINUTE_TO_SECOND = 60;
} // namespace

void TemporaryColorModeManager::InitData(const int32_t userId)
{
    InitData(AccountContextHelper::CreateBaseContext(userId));
}

void TemporaryColorModeManager::InitData(const AccountContext& context)
{
    TempColorModeInfo info;
    std::string temporaryColorModeValue = TEMPORARY_COLOR_MODE_NORMAL_STRING;
    GetParameterWrap(TemporaryColorModeAssignUser(context), temporaryColorModeValue);
    info.tempColorMode = temporaryColorModeValue == TEMPORARY_COLOR_MODE_NORMAL_STRING
                                ? TempColorModeType::ColorModeNormal
                                : TempColorModeType::ColorModeTemp;
    if (info.tempColorMode == TempColorModeType::ColorModeTemp) {
        std::string startTime = "0";
        GetParameterWrap(TemporaryStateStartTimeAssignUser(context), startTime);
        info.keepTemporaryStateStartTime = atoll(startTime.c_str());

        std::string endTime = "0";
        GetParameterWrap(TemporaryStateEndTimeAssignUser(context), endTime);
        info.keepTemporaryStateEndTime = atoll(endTime.c_str());
    }
    {
        std::lock_guard guard(multiUserTempColorModeMapMutex_);
        multiUserTempColorModeMap_[context] = info;
    }
    LOGI("init temp colormode info context:%{public}s, tempColorMode:%{public}d, keepStartTime:%{public}" PRId64
         ", keepEndTime:%{public}" PRId64, AccountContextHelper::ToString(context).c_str(),
        static_cast<int32_t>(info.tempColorMode), info.keepTemporaryStateStartTime,
        info.keepTemporaryStateEndTime);
}

bool TemporaryColorModeManager::IsColorModeTemporary(const int32_t userId)
{
    return IsColorModeTemporary(AccountContextHelper::CreateBaseContext(userId));
}

bool TemporaryColorModeManager::IsColorModeTemporary(const AccountContext& context)
{
    std::lock_guard guard(multiUserTempColorModeMapMutex_);
    auto it = multiUserTempColorModeMap_.find(context);
    if (it != multiUserTempColorModeMap_.end()) {
        return it->second.tempColorMode == TempColorModeType::ColorModeTemp;
    }
    return false;
}
bool TemporaryColorModeManager::IsColorModeNormal(const int32_t userId)
{
    return IsColorModeNormal(AccountContextHelper::CreateBaseContext(userId));
}

bool TemporaryColorModeManager::IsColorModeNormal(const AccountContext& context)
{
    std::lock_guard guard(multiUserTempColorModeMapMutex_);
    auto it = multiUserTempColorModeMap_.find(context);
    if (it != multiUserTempColorModeMap_.end()) {
        return it->second.tempColorMode == TempColorModeType::ColorModeNormal;
    }
    return true;
}
bool TemporaryColorModeManager::SetColorModeTemporary(const int32_t userId)
{
    return SetColorModeTemporary(AccountContextHelper::CreateBaseContext(userId));
}

bool TemporaryColorModeManager::SetColorModeTemporary(const AccountContext& context)
{
    {
        std::lock_guard guard(multiUserTempColorModeMapMutex_);
        multiUserTempColorModeMap_[context].tempColorMode = TempColorModeType::ColorModeTemp;
        int32_t settingStartTime = 0;
        int32_t settingEndTime = 0;
        auto res = DarkModeManager::GetInstance().GetSettingTime(context, settingStartTime, settingEndTime);
        if (res == false) {
            LOGE("GetSettingTime faild context: %{public}s", AccountContextHelper::ToString(context).c_str());
            return false;
        }
        GetTempColorModeTimeInfo(settingStartTime, settingEndTime,
            multiUserTempColorModeMap_[context].keepTemporaryStateStartTime,
            multiUserTempColorModeMap_[context].keepTemporaryStateEndTime);
    }
    SaveTempColorModeInfo(context);
    return true;
}
bool TemporaryColorModeManager::SetColorModeNormal(const int32_t userId)
{
    return SetColorModeNormal(AccountContextHelper::CreateBaseContext(userId));
}

bool TemporaryColorModeManager::SetColorModeNormal(const AccountContext& context)
{
    {
        std::lock_guard guard(multiUserTempColorModeMapMutex_);
        multiUserTempColorModeMap_[context].tempColorMode = TempColorModeType::ColorModeNormal;
        multiUserTempColorModeMap_[context].keepTemporaryStateStartTime = 0;
        multiUserTempColorModeMap_[context].keepTemporaryStateEndTime = 0;
    }
    SaveTempColorModeInfo(context);
    return true;
}

bool TemporaryColorModeManager::CheckTemporaryStateEffective(const int32_t userId)
{
    return CheckTemporaryStateEffective(AccountContextHelper::CreateBaseContext(userId));
}

bool TemporaryColorModeManager::CheckTemporaryStateEffective(const AccountContext& context)
{
    auto checkTempStateNoTimeout = [](const int64_t startTime, const int64_t endTime) {
        std::time_t timestampNow = std::time(nullptr);
        if (timestampNow == static_cast<std::time_t>(-1)) {
            LOGE("fail to get timestamp");
            return false;
        }
        if (startTime < timestampNow && timestampNow < endTime) {
            return true;
        }
        return false;
    };

    std::lock_guard guard(multiUserTempColorModeMapMutex_);
    auto it = multiUserTempColorModeMap_.find(context);
    if (it != multiUserTempColorModeMap_.end()) {
        if (it->second.tempColorMode == TempColorModeType::ColorModeTemp &&
            checkTempStateNoTimeout(it->second.keepTemporaryStateStartTime, it->second.keepTemporaryStateEndTime)) {
            return true;
        }
    }
    return false;
}

std::string TemporaryColorModeManager::TemporaryColorModeAssignUser(const AccountContext& context)
{
    return AccountContextHelper::BuildUserParamKey(TEMPORARY_COLOR_MODE_PARAM_STRING, context);
}

std::string TemporaryColorModeManager::TemporaryStateStartTimeAssignUser(const AccountContext& context)
{
    return AccountContextHelper::BuildUserParamKey(TEMPORARY_STATE_START_TIME_PARAM_STRING, context);
}

std::string TemporaryColorModeManager::TemporaryStateEndTimeAssignUser(const AccountContext& context)
{
    return AccountContextHelper::BuildUserParamKey(TEMPORARY_STATE_END_TIME_PARAM_STRING, context);
}

void TemporaryColorModeManager::GetTempColorModeTimeInfo(const int32_t settingStartTime,
    const int32_t settingEndTime, int64_t& tempStateStartTime, int64_t& tempStateEndTime)
{
    auto calcEndTimestamp = [](const int32_t secondOffset, int64_t& endTimeStamp) {
        std::time_t timestamp = std::time(nullptr);
        if (timestamp == static_cast<std::time_t>(-1)) {
            LOGE("fail to get timestamp");
            return false;
        }
        std::tm* nowTime = std::localtime(&timestamp);
        if (nowTime != nullptr) {
            nowTime->tm_hour = 0;
            nowTime->tm_min = 0;
            nowTime->tm_sec = 0;
        }
        std::time_t midnightTime = std::mktime(nowTime);
        endTimeStamp = midnightTime + secondOffset - 1;
        return true;
    };

    tempStateStartTime = static_cast<int64_t>(std::time(nullptr));

    if (settingEndTime > DAY_TO_MINUTE) {
        if (AlarmTimerManager::IsWithinTimeInterval(settingStartTime, settingEndTime) == false) {
            /*********************************************************************
            for example settingStartTime=20, settingEndTime=8
                        ↓(setting)
            xxxxxxxxx           xxxxxxxxxxxxxxxxxxxxxxxxxxxx            xxxxxxxxxx
            0		8			20			24(0)			8		   20       24
                        |-----------------------------------|
            ************************************************************************/
            calcEndTimestamp((settingEndTime * MINUTE_TO_SECOND), tempStateEndTime);
        } else if (TemporaryColorModeManager::IsWithInPreInterval(settingStartTime, settingEndTime)) {
            /*********************************************************************
            for example settingStartTime=20, settingEndTime=8
              ↓(setting)
            xxxxxxxxx           xxxxxxxxxxxxxxxxxxxxxxxxxxxx            xxxxxxxxxx
            0		8			20			24(0)			8		   20       24
              |-----------------|
            ************************************************************************/
            calcEndTimestamp((settingStartTime * MINUTE_TO_SECOND), tempStateEndTime);
        } else {
            /*********************************************************************
            for example settingStartTime=20, settingEndTime=8
                                       ↓(setting)
            xxxxxxxxx           xxxxxxxxxxxxxxxxxxxxxxxxxxxx            xxxxxxxxxx
            0		8			20			24(0)			8		   20       24
                                       |-------------------------------|
            ************************************************************************/
            calcEndTimestamp(((settingStartTime + DAY_TO_MINUTE) * MINUTE_TO_SECOND), tempStateEndTime);
        }
    } else {
        if (AlarmTimerManager::IsWithinTimeInterval(settingStartTime, settingEndTime)) {
            /*********************************************************************
            for example settingStartTime=11, settingEndTime=16
                        ↓(setting)
                    xxxxxxxxxx                              xxxxxxxxxxxx
            0		11		16				24(0)			11		   16       24
                        |-----------------------------------|
            ************************************************************************/
            calcEndTimestamp(((settingStartTime + DAY_TO_MINUTE) * MINUTE_TO_SECOND), tempStateEndTime);
        } else if (TemporaryColorModeManager::IsWithInPreInterval(settingStartTime, settingEndTime)) {
            /*********************************************************************
            for example settingStartTime=11, settingEndTime=16
               ↓(setting)
                    xxxxxxxxxx                              xxxxxxxxxxxx
            0		11		16				24(0)			11		   16       24
               |-------------|
            ************************************************************************/
            calcEndTimestamp((settingEndTime * MINUTE_TO_SECOND), tempStateEndTime);
        } else {
            /*********************************************************************
            for example settingStartTime=11, settingEndTime=16
                                  ↓(setting)
                    xxxxxxxxxx                              xxxxxxxxxxxx
            0		11		16				24(0)			11		   16       24
                                  |-------------------------------------|
            ************************************************************************/
            calcEndTimestamp(((settingEndTime + DAY_TO_MINUTE) * MINUTE_TO_SECOND), tempStateEndTime);
        }
    }
}

bool TemporaryColorModeManager::IsWithInPreInterval(const int32_t startTime, const int32_t endTime)
{
    std::time_t timestamp = std::time(nullptr);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("fail to get timestamp");
        return false;
    }
    std::tm* nowTime = std::localtime(&timestamp);
    int32_t totalMinutes { 0 };
    if (nowTime != nullptr) {
        totalMinutes = static_cast<int32_t>(nowTime->tm_hour * HOUR_TO_MINUTE + nowTime->tm_min);
    }

    if (totalMinutes <= startTime) {
        return true;
    }
    return false;
}

void TemporaryColorModeManager::SaveTempColorModeInfo(const AccountContext& context)
{
    TempColorModeInfo info;
    {
        std::lock_guard guard(multiUserTempColorModeMapMutex_);
        info = multiUserTempColorModeMap_[context];
    }
    SetParameterWrap(TemporaryColorModeAssignUser(context), info.tempColorMode == TempColorModeType::ColorModeNormal
                                                               ? TEMPORARY_COLOR_MODE_NORMAL_STRING
                                                               : TEMPORARY_COLOR_MODE_TEMPORARY_STRING);
    LOGI("SaveTempColorModeInfo context:%{public}s,colorMode:%{public}d",
        AccountContextHelper::ToString(context).c_str(), static_cast<int32_t>(info.tempColorMode));
    if (info.tempColorMode == TempColorModeType::ColorModeTemp) {
        SetParameterWrap(TemporaryStateStartTimeAssignUser(context),
            std::to_string(info.keepTemporaryStateStartTime));
        SetParameterWrap(TemporaryStateEndTimeAssignUser(context), std::to_string(info.keepTemporaryStateEndTime));
        LOGI("SaveTempColorModeInfo keepStartTime:%{public}" PRId64 ",keepEndTime:%{public}" PRId64,
            info.keepTemporaryStateStartTime, info.keepTemporaryStateEndTime);
    }
}

} // namespace OHOS::ArkUi::UiAppearance
