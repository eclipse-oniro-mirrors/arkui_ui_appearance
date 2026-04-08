/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#include "ui_appearance_ability.h"

#include <cstdlib>
#include <string>

#include "accesstoken_kit.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "dark_mode_manager.h"
#include "global_configuration_key.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "matching_skills.h"
#include "os_account_manager.h"
#include "smart_gesture_manager.h"
#include "system_ability_definition.h"
#include "ui_appearance_log.h"
#include "parameter_wrap.h"
#include "background_app_color_switch_settings.h"
#include "background_app_info.h"
#include "configuration_policy.h"

namespace {
static const std::string LIGHT = "light";
static const std::string DARK = "dark";
static const std::string BASE_SCALE = "1";
static const std::string STANDARD_FONT_WEIGHT = "const.standard_font_weight";
static const std::string PERSIST_DARKMODE_KEY = "persist.ace.darkmode";
static const std::string PERMISSION_UPDATE_CONFIGURATION = "ohos.permission.UPDATE_CONFIGURATION";
// current default accountId = 0, will change when have more user.
static const std::string FONT_SCAL_FOR_USER0 = "persist.sys.font_scale_for_user0";
static const std::string FONT_Weight_SCAL_FOR_USER0 = "persist.sys.font_wght_scale_for_user0";

static const std::string PERSIST_DARKMODE_KEY_FOR_NONE = "persist.ace.darkmode.";
static const std::string FONT_SCAL_FOR_NONE = "persist.sys.font_scale_for_user.";
static const std::string FONT_WEIGHT_SCAL_FOR_NONE = "persist.sys.font_wght_scale_for_user.";

static const std::string FIRST_INITIALIZATION = "persist.uiAppearance.first_initialization";
const static int32_t USER100 = 100;
const static int32_t USER0 = 0;
const static std::string FIRST_UPGRADE = "1";
const static std::string NOT_FIRST_UPGRADE = "0";
} // namespace

namespace OHOS {
namespace ArkUi::UiAppearance {
namespace {
static constexpr double MIN_FONT_SCALE = 0.0;
static constexpr double MAX_FONT_SCALE = 5.0;

bool IsValidFontWeightScaleString(const std::string& value)
{
    if (value.empty()) {
        return false;
    }

    errno = 0;
    char* endPtr = nullptr;
    double fontWeightScale = std::strtod(value.c_str(), &endPtr);
    if (endPtr == value.c_str() || endPtr == nullptr || *endPtr != '\0' || errno == ERANGE) {
        return false;
    }
    return fontWeightScale > MIN_FONT_SCALE && fontWeightScale <= MAX_FONT_SCALE;
}

std::string GetDefaultFontWeightScaleValue(const std::string& defaultValue)
{
    std::string defaultFontWeightScale = defaultValue;
    if (GetParameterWrap(STANDARD_FONT_WEIGHT, defaultFontWeightScale)) {
        LOGI("get default fontWeightScale from %{public}s, value:%{public}s", STANDARD_FONT_WEIGHT.c_str(),
            defaultFontWeightScale.c_str());
        if (!IsValidFontWeightScaleString(defaultFontWeightScale)) {
            LOGW("invalid %{public}s value:%{public}s, fallback to defaultValue:%{public}s",
                STANDARD_FONT_WEIGHT.c_str(), defaultFontWeightScale.c_str(), defaultValue.c_str());
            return defaultValue;
        }
    }
    return defaultFontWeightScale;
}
} // namespace

UiAppearanceAbility::UiAppearanceParam::UiAppearanceParam()
    : fontWeightScale(GetDefaultFontWeightScaleValue(BASE_SCALE))
{}

UiAppearanceEventSubscriber::UiAppearanceEventSubscriber(const EventFwk::CommonEventSubscribeInfo& subscriberInfo,
    const std::function<void(const int32_t)>& userSwitchCallback)
    : EventFwk::CommonEventSubscriber(subscriberInfo), userSwitchCallback_(userSwitchCallback)
{}

void UiAppearanceEventSubscriber::OnReceiveEvent(const EventFwk::CommonEventData& data)
{
    const AAFwk::Want& want = data.GetWant();
    std::string action = want.GetAction();
    LOGI("action:%{public}s", action.c_str());

    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        if (userSwitchCallback_ != nullptr) {
            userSwitchCallback_(data.GetCode());
        }
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_TIME_CHANGED) {
        TimeChangeCallback();
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_TIMEZONE_CHANGED) {
        TimeChangeCallback();
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED) {
        BootCompetedCallback();
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
        DarkModeManager::GetInstance().ScreenOffCallback();
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON) {
        DarkModeManager::GetInstance().ScreenOnCallback();
    }
}

void UiAppearanceEventSubscriber::TimeChangeCallback()
{
    DarkModeManager::GetInstance().RestartTimer();
}

void UiAppearanceEventSubscriber::BootCompetedCallback()
{
    std::call_once(bootCompleteFlag_, [] () {
        std::vector<int32_t> ids;
        AccountSA::OsAccountManager::QueryActiveOsAccountIds(ids);
        int32_t userId;
        if (ids.empty()) {
            LOGE("no active user.");
            userId = USER100;
        } else {
            userId = ids[0];
        }
        DarkModeManager &manager = DarkModeManager::GetInstance();
        manager.OnSwitchUser(userId);
        bool isDarkMode = false;
        manager.LoadUserSettingData(userId, true, isDarkMode, true);
        SmartGestureManager &smartGestureManager = SmartGestureManager::GetInstance();
        smartGestureManager.RegisterSettingDataObserver();
        smartGestureManager.UpdateSmartGestureInitialValue();
    });
}

REGISTER_SYSTEM_ABILITY_BY_ID(UiAppearanceAbility, ARKUI_UI_APPEARANCE_SERVICE_ID, true);

UiAppearanceAbility::UiAppearanceAbility(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate) {}

sptr<AppExecFwk::IAppMgr> UiAppearanceAbility::GetAppManagerInstance()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        LOGE("Getting systemAbilityManager failed.");
        return nullptr;
    }

    sptr<IRemoteObject> appObject = systemAbilityManager->GetSystemAbility(APP_MGR_SERVICE_ID);
    if (appObject == nullptr) {
        LOGE("Get systemAbility failed.");
        return nullptr;
    }

    sptr<AppExecFwk::IAppMgr> systemAbility = iface_cast<AppExecFwk::IAppMgr>(appObject);
    if (systemAbility == nullptr) {
        LOGE("Get AppMgrProxy from SA failed.");
        return nullptr;
    }
    return systemAbility;
}

bool UiAppearanceAbility::VerifyAccessToken(const std::string& permissionName)
{
    auto callerToken = IPCSkeleton::GetCallingTokenID();
    int32_t ret = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerToken, permissionName);
    if (ret == Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        return true;
    }
    LOGE("permission %{private}s denied, callerToken : %{public}u", permissionName.c_str(), callerToken);
    return false;
}

void UiAppearanceAbility::OnStart()
{
    bool res = Publish(this); // SA registers with SAMGR
    if (!res) {
        LOGE("publish failed.");
        return;
    }

    LOGI("AddSystemAbilityListener start.");
    AddSystemAbilityListener(APP_MGR_SERVICE_ID);
    return;
}

void UiAppearanceAbility::OnStop()
{
    LOGI("UiAppearanceAbility SA stop.");
}

std::list<int32_t> UiAppearanceAbility::GetUserIds()
{
    std::vector<AccountSA::OsAccountInfo> infos;
    auto errCode = AccountSA::OsAccountManager::QueryAllCreatedOsAccounts(infos);
    if (errCode != 0) {
        LOGW("QueryAllCreatedOsAccounts error: %{public}d.", errCode);
        return {};
    }
    std::list<int32_t> ids;
    for (const auto& info : infos) {
        ids.push_back(info.GetLocalId());
    }
    return ids;
}

void UiAppearanceAbility::DoCompatibleProcess()
{
    LOGI("DoCompatibleProcess");
    auto getOldParam = [this](const std::string& paramName, std::string& result) {
        return GetParameterWrap(paramName, result);
    };

    auto isParamAllreadaySetted = [this](const std::string& paramName) {
        std::string value;
        return GetParameterWrap(paramName, value);
    };

    const std::list<int32_t> userIds = GetUserIds();
    std::string darkMode = LIGHT;
    if (getOldParam(PERSIST_DARKMODE_KEY, darkMode)) {
        for (auto id : userIds) {
            if (isParamAllreadaySetted(DarkModeParamAssignUser(id))) {
                continue;
            }
            SetParameterWrap(DarkModeParamAssignUser(id), darkMode);
            LOGI("userId:%{public}d set darkMode %{public}s", id, darkMode.c_str());
        }
    }
    std::string fontSize = BASE_SCALE;
    if (getOldParam(FONT_SCAL_FOR_USER0, fontSize)) {
        for (auto id : userIds) {
            if (isParamAllreadaySetted(FontScaleParamAssignUser(id))) {
                continue;
            }
            SetParameterWrap(FontScaleParamAssignUser(id), fontSize);
            LOGI("userId:%{public}d set fontSize %{public}s", id, fontSize.c_str());
        }
    }
    std::string fontWeightSize = BASE_SCALE;
    if (getOldParam(FONT_Weight_SCAL_FOR_USER0, fontWeightSize)) {
        fontWeightSize = GetDefaultFontWeightScaleValue(fontWeightSize);
        for (auto id : userIds) {
            if (isParamAllreadaySetted(FontWeightScaleParamAssignUser(id))) {
                continue;
            }
            SetParameterWrap(FontWeightScaleParamAssignUser(id), fontWeightSize);
            LOGI("userId:%{public}d set fontWeightSize %{public}s", id, fontWeightSize.c_str());
        }
    }
    SetParameterWrap(FIRST_INITIALIZATION, "0");
    isNeedDoCompatibleProcess_ = false;
}

void UiAppearanceAbility::DoInitProcess()
{
    LOGI("DoInitProcess");
    BackGroundAppColorSwitchSettings::GetInstance().Initialize();
    const std::list<int32_t> userIds = GetUserIds();
    for (auto userId : userIds) {
        std::string darkValue = LIGHT;
        GetParameterWrap(DarkModeParamAssignUser(userId), darkValue);

        std::string fontSize = BASE_SCALE;
        GetParameterWrap(FontScaleParamAssignUser(userId), fontSize);

        std::string fontWeight = GetDefaultFontWeightScaleValue(BASE_SCALE);
        GetParameterWrap(FontWeightScaleParamAssignUser(userId), fontWeight);

        UiAppearanceParam tmpParam;
        tmpParam.darkMode = darkValue == DARK ? DarkMode::ALWAYS_DARK : DarkMode::ALWAYS_LIGHT;
        tmpParam.fontScale = fontSize;
        tmpParam.fontWeightScale = fontWeight;
        {
            std::lock_guard<std::mutex> guard(usersParamMutex_);
            usersParam_[userId] = tmpParam;
        }
        LOGI("init userId:%{public}d, darkMode:%{public}s, fontSize:%{public}s, fontWeight:%{public}s", userId,
            darkValue.c_str(), fontSize.c_str(), fontWeight.c_str());
    }
    isInitializationFinished_ = true;
}

void UiAppearanceAbility::UpdateCurrentUserConfiguration(const int32_t userId, const bool isForceUpdate)
{
    UiAppearanceParam tmpParam;
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        tmpParam = usersParam_[userId];
    }
    AppExecFwk::Configuration config;
    config.AddItem(
        AAFwk::GlobalConfigurationKey::SYSTEM_COLORMODE, tmpParam.darkMode == DarkMode::ALWAYS_DARK ? DARK : LIGHT);
    config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_SIZE_SCALE, tmpParam.fontScale);
    config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_WEIGHT_SCALE, tmpParam.fontWeightScale);

    auto appManagerInstance = GetAppManagerInstance();
    if (!appManagerInstance) {
        LOGE("GetAppManagerInstance error userId:%{public}d", userId);
        return;
    }

    {
        std::lock_guard<std::mutex> onceFlagGuard(userSwitchUpdateConfigurationOnceFlagMutex_);
        if (isForceUpdate ||
            userSwitchUpdateConfigurationOnceFlag_.find(userId) == userSwitchUpdateConfigurationOnceFlag_.end()) {
            appManagerInstance->UpdateConfiguration(config, userId);
            LOGI("update userId:%{public}d configuration:%{public}s", userId, config.GetName().c_str());
            userSwitchUpdateConfigurationOnceFlag_.insert(userId);
        } else {
            appManagerInstance->UpdateConfiguration(config, USER0);
            LOGI("update userId:%{public}d configuration:%{public}s", USER0, config.GetName().c_str());
        }
    }

    SetParameterWrap(PERSIST_DARKMODE_KEY, tmpParam.darkMode == DarkMode::ALWAYS_DARK ? DARK : LIGHT);
    SetParameterWrap(FONT_SCAL_FOR_USER0, tmpParam.fontScale);
    SetParameterWrap(FONT_Weight_SCAL_FOR_USER0, tmpParam.fontWeightScale);
}

void UiAppearanceAbility::UserSwitchFunc(const int32_t userId)
{
    DarkModeManager& manager = DarkModeManager::GetInstance();
    manager.OnSwitchUser(userId);
    bool isDarkMode = false;
    int32_t code = manager.LoadUserSettingData(userId, false, isDarkMode, false);

    if (isNeedDoCompatibleProcess_) {
        DoCompatibleProcess();
    }
    if (!isInitializationFinished_) {
        DoInitProcess();
    }

    bool isForceUpdate = false;
    if (code == ERR_OK && manager.IsColorModeNormal(userId)) {
        DarkMode darkMode = isDarkMode ? ALWAYS_DARK : ALWAYS_LIGHT;
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        if (usersParam_[userId].darkMode != darkMode) {
            usersParam_[userId].darkMode = darkMode;
            isForceUpdate = true;
        }
    }

    UpdateCurrentUserConfiguration(userId, isForceUpdate);
}

void UiAppearanceAbility::SubscribeCommonEvent()
{
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_BOOT_COMPLETED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_TIME_CHANGED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_TIMEZONE_CHANGED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);

    uiAppearanceEventSubscriber_ = std::make_shared<UiAppearanceEventSubscriber>(
        subscribeInfo, [this](const int32_t userId) { UserSwitchFunc(userId); });
    bool subResult = EventFwk::CommonEventManager::SubscribeCommonEvent(uiAppearanceEventSubscriber_);
    if (!subResult) {
        LOGW("subscribe user switch event error");
    }
}

void UiAppearanceAbility::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    if (systemAbilityId != APP_MGR_SERVICE_ID) {
        return;
    }

    auto checkIfFirstUpgrade = [this]() {
        std::string initFlag = NOT_FIRST_UPGRADE;
        GetParameterWrap(FIRST_INITIALIZATION, initFlag);
        if (initFlag == FIRST_UPGRADE) {
            return true;
        }
        return false;
    };
    isNeedDoCompatibleProcess_ = checkIfFirstUpgrade();
    DarkModeManager::GetInstance().Initialize(
        [this](bool isDarkMode, int32_t userId) { UpdateDarkModeCallback(isDarkMode, userId); });
    SmartGestureManager::GetInstance().Initialize(
        [this](bool isAutoMode, int32_t userId) { UpdateSmartGestureModeCallback(isAutoMode, userId); });
    SubscribeCommonEvent();
    if (isNeedDoCompatibleProcess_ && !GetUserIds().empty()) {
        DoCompatibleProcess();
    }

    if (!isInitializationFinished_ && !GetUserIds().empty()) {
        DoInitProcess();
        int32_t userId = USER100;
        auto errCode = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
        if (errCode != 0) {
            LOGW("GetForegroundOsAccountLocalId error: %{public}d.", errCode);
            userId = USER100;
        }
        UpdateCurrentUserConfiguration(userId, false);
    }
}

void UiAppearanceAbility::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    LOGI("systemAbilityId = %{public}d removed.", systemAbilityId);
    if (systemAbilityId == APP_MGR_SERVICE_ID) {
        std::lock_guard<std::mutex> onceFlagGuard(userSwitchUpdateConfigurationOnceFlagMutex_);
        userSwitchUpdateConfigurationOnceFlag_.clear();
    }
}

int32_t UiAppearanceAbility::GetCallingUserId()
{
    const static int32_t UID_TRANSFORM_DIVISOR = 200000;

    LOGD("CallingUid = %{public}d", OHOS::IPCSkeleton::GetCallingUid());
    int32_t userId = OHOS::IPCSkeleton::GetCallingUid() / UID_TRANSFORM_DIVISOR;
    if (userId == 0) {
        auto errNo = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(userId);
        if (errNo != 0) {
            LOGE("CallingUid = %{public}d, GetForegroundOsAccountLocalId error:%{public}d",
                OHOS::IPCSkeleton::GetCallingUid(), errNo);
            userId = USER100;
        }
    }
    return userId;
}

std::string UiAppearanceAbility::DarkModeParamAssignUser(const int32_t userId)
{
    return PERSIST_DARKMODE_KEY_FOR_NONE + std::to_string(userId);
}
std::string UiAppearanceAbility::FontScaleParamAssignUser(const int32_t userId)
{
    return FONT_SCAL_FOR_NONE + std::to_string(userId);
}
std::string UiAppearanceAbility::FontWeightScaleParamAssignUser(const int32_t userId)
{
    return FONT_WEIGHT_SCAL_FOR_NONE + std::to_string(userId);
}

bool UiAppearanceAbility::BackGroundAppColorSwitch(sptr<AppExecFwk::IAppMgr> appManagerInstance, const int32_t userId)
{
    if (!BackGroundAppColorSwitchSettings::GetInstance().IsSupportHotUpdate()) {
        LOGI("not Support BackGround App Color Switch");
        return false;
    }

    std::vector<AppExecFwk::BackgroundAppInfo> backgroundAppInfoVe;
    for (const auto& whiteListItem : BackGroundAppColorSwitchSettings::GetInstance().GetWhileList()) {
        AppExecFwk::BackgroundAppInfo appInfo;
        appInfo.bandleName = whiteListItem;
        appInfo.appIndex = 0;
        backgroundAppInfoVe.push_back(appInfo);
    }

    if (backgroundAppInfoVe.empty()) {
        LOGD("no need backGround app color Switch");
        return true;
    }

    AppExecFwk::ConfigurationPolicy policy;
    policy.maxCountPerBatch  = BackGroundAppColorSwitchSettings::GetInstance().GetTaskQuantity();
    policy.intervalTime = BackGroundAppColorSwitchSettings::GetInstance().GetDurationMillisecond();
    LOGI("BackGroundAppColorSwitch settings maxCountPerBatch :%{public}d intervalTime :%{public}d.",
        BackGroundAppColorSwitchSettings::GetInstance().GetTaskQuantity(),
        BackGroundAppColorSwitchSettings::GetInstance().GetDurationMillisecond());
    auto result = appManagerInstance->UpdateConfigurationForBackgroundApp(backgroundAppInfoVe, policy, userId);
    if (!result) {
        LOGE("UpdateConfigurationForBackgroundApp fail result :%{public}d.", result);
        return false;
    }
    return true;
}

bool UiAppearanceAbility::UpdateConfiguration(const AppExecFwk::Configuration& configuration, const int32_t userId,
    const std::vector<std::int32_t>& effectiveUserIds)
{
    auto appManagerInstance = GetAppManagerInstance();
    if (appManagerInstance == nullptr) {
        LOGE("Get app manager proxy failed.");
        return false;
    }

    int32_t errcode = 0;
    if (effectiveUserIds.size() > 1) {
        LOGI("UpdateConfigurationByUserIds start, config = %{public}s.", configuration.GetName().c_str());
        errcode = appManagerInstance->UpdateConfigurationByUserIds(configuration, effectiveUserIds);
    } else {
        LOGI("update Configuration start,userId:%{public}d config = %{public}s.",
            userId, configuration.GetName().c_str());
        errcode = appManagerInstance->UpdateConfiguration(configuration, userId);
    }

    if (errcode != 0) {
        AppExecFwk::Configuration config;
        auto retVal = appManagerInstance->GetConfiguration(config);
        if (retVal != 0) {
            LOGE("get configuration failed, update error, error is %{public}d.", retVal);
            return false;
        }
        std::vector<std::string> diffVe;
        config.CompareDifferent(diffVe, configuration);

        if (!diffVe.empty()) {
            LOGE("update configuration failed, errcode = %{public}d.", errcode);
            return false;
        } else {
            LOGW("uiappearance is different against configuration. Forced to use the configuration, error is "
                "%{public}d.", errcode);
        }
    } else if (!configuration.GetItem(AAFwk::GlobalConfigurationKey::SYSTEM_COLORMODE).empty()) {
        if (effectiveUserIds.size() > 1) {
            for (const int32_t &effectiveUserId : effectiveUserIds) {
                BackGroundAppColorSwitch(appManagerInstance, effectiveUserId);
            }
        } else {
            BackGroundAppColorSwitch(appManagerInstance, userId);
        }
    }
    return true;
}

int32_t UiAppearanceAbility::OnSetDarkMode(const int32_t userId, DarkMode mode)
{
    LOGI("setDarkMode, userId:%{public}d, mode: %{public}d", userId, mode);
    bool ret = false;
    std::string paramValue;
    AppExecFwk::Configuration config;
    switch (mode) {
        case ALWAYS_LIGHT: {
            ret = config.AddItem(
                AAFwk::GlobalConfigurationKey::SYSTEM_COLORMODE, AppExecFwk::ConfigurationInner::COLOR_MODE_LIGHT);
            paramValue.assign(LIGHT);
            break;
        }
        case ALWAYS_DARK: {
            ret = config.AddItem(
                AAFwk::GlobalConfigurationKey::SYSTEM_COLORMODE, AppExecFwk::ConfigurationInner::COLOR_MODE_DARK);
            paramValue.assign(DARK);
            break;
        }
        default:
            break;
    }
    if (!ret) {
        LOGE("AddItem failed, mode = %{public}d", mode);
        return INVALID_ARG;
    }

    std::vector<int32_t> effectiveUserIds = GetMultipleUsers();
    if (!UpdateConfiguration(config, userId, effectiveUserIds)) {
        return SYS_ERR;
    }

    if (effectiveUserIds.size() > 1) {
        SetParameterWrap(PERSIST_DARKMODE_KEY, paramValue);
        for (const int32_t effectiveUserId : effectiveUserIds) {
            if (ConfigurePersistence(effectiveUserId, mode, paramValue) != SUCCEEDED) {
                return SYS_ERR;
            }
        }
        return SUCCEEDED;
    }

    SetParameterWrap(PERSIST_DARKMODE_KEY, paramValue);
    return ConfigurePersistence(userId, mode, paramValue);
}

ErrCode UiAppearanceAbility::SetDarkMode(int32_t mode, int32_t& funcResult)
{
    // Verify permissions
    DarkMode darkMode = static_cast<DarkMode>(mode);
    auto isCallingPerm = VerifyAccessToken(PERMISSION_UPDATE_CONFIGURATION);
    if (!isCallingPerm) {
        LOGE("permission verification failed");
        funcResult = PERMISSION_ERR;
        return SUCCEEDED;
    }

    auto userId = GetCallingUserId();
    DarkMode currentDarkMode = DarkMode::ALWAYS_LIGHT;
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        auto it = usersParam_.find(userId);
        if (it != usersParam_.end()) {
            currentDarkMode = it->second.darkMode;
        }
    }
    if (darkMode != currentDarkMode) {
        funcResult = OnSetDarkMode(userId, darkMode);
        return SUCCEEDED;
    } else {
        LOGW("current color mode is %{public}d, no need to change", darkMode);
        funcResult = SYS_ERR;
        return SUCCEEDED;
    }
}

DarkMode UiAppearanceAbility::InitGetDarkMode(const int32_t userId)
{
    std::string valueGet = LIGHT;

    // LIGHT is the default.
    auto res = GetParameterWrap(DarkModeParamAssignUser(userId), valueGet);
    if (!res) {
        return ALWAYS_LIGHT;
    }
    if (valueGet == DARK) {
        LOGI("current color mode is dark.");
        return ALWAYS_DARK;
    } else if (valueGet == LIGHT) {
        LOGI("current color mode is light.");
        return ALWAYS_LIGHT;
    }
    return ALWAYS_LIGHT;
}

ErrCode UiAppearanceAbility::GetDarkMode(int32_t& funcResult)
{
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        auto it = usersParam_.find(GetCallingUserId());
        if (it != usersParam_.end()) {
            funcResult = it->second.darkMode;
            return SUCCEEDED;
        }
    }

    funcResult = DarkMode::ALWAYS_LIGHT;
    return SUCCEEDED;
}

int32_t UiAppearanceAbility::OnSetFontScale(const int32_t userId, const std::string& fontScale)
{
    bool ret = false;
    AppExecFwk::Configuration config;
    ret = config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_SIZE_SCALE, fontScale);
    if (!ret) {
        LOGE("AddItem failed, fontScale = %{public}s", fontScale.c_str());
        return INVALID_ARG;
    }
    if (!UpdateConfiguration(config, userId)) {
        return SYS_ERR;
    }

    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[userId].fontScale = fontScale;
    }

    SetParameterWrap(FONT_SCAL_FOR_USER0, fontScale);
    // persist to file: etc/para/ui_appearance.para
    auto isSetPara = SetParameterWrap(FontScaleParamAssignUser(userId), fontScale);
    if (!isSetPara) {
        LOGE("set parameter failed");
        return SYS_ERR;
    }
    return SUCCEEDED;
}

ErrCode UiAppearanceAbility::SetFontScale(const std::string& fontScale, int32_t& funcResult)
{
    // Verify permissions
    auto isCallingPerm = VerifyAccessToken(PERMISSION_UPDATE_CONFIGURATION);
    if (!isCallingPerm) {
        LOGE("permission verification failed");
        funcResult = PERMISSION_ERR;
        return SUCCEEDED;
    }
    if (!fontScale.empty()) {
        funcResult = OnSetFontScale(GetCallingUserId(), fontScale);
        return SUCCEEDED;
    } else {
        LOGE("current fontScale is empty!");
    }
    funcResult = SYS_ERR;
    return SUCCEEDED;
}

ErrCode UiAppearanceAbility::GetFontScale(std::string& fontScale, int32_t& funcResult)
{
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        auto it = usersParam_.find(GetCallingUserId());
        if (it != usersParam_.end()) {
            fontScale = it->second.fontScale;
        } else {
            fontScale = BASE_SCALE;
        }
    }
    LOGD("get font scale :%{public}s", fontScale.c_str());
    funcResult = SUCCEEDED;
    return SUCCEEDED;
}

int32_t UiAppearanceAbility::OnSetFontWeightScale(const int32_t userId, const std::string& fontWeightScale)
{
    bool ret = false;
    AppExecFwk::Configuration config;
    ret = config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_WEIGHT_SCALE, fontWeightScale);
    if (!ret) {
        LOGE("AddItem failed, fontWeightScale = %{public}s", fontWeightScale.c_str());
        return INVALID_ARG;
    }

    if (!UpdateConfiguration(config, userId)) {
        return SYS_ERR;
    }
    
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[userId].fontWeightScale = fontWeightScale;
    }

    SetParameterWrap(FONT_Weight_SCAL_FOR_USER0, fontWeightScale);

    // persist to file: etc/para/ui_appearance.para
    auto isSetPara = SetParameterWrap(FontWeightScaleParamAssignUser(userId), fontWeightScale);
    if (!isSetPara) {
        LOGE("set parameter failed");
        return SYS_ERR;
    }
    return SUCCEEDED;
}

ErrCode UiAppearanceAbility::SetFontWeightScale(const std::string& fontWeightScale, int32_t& funcResult)
{
    // Verify permissions
    auto isCallingPerm = VerifyAccessToken(PERMISSION_UPDATE_CONFIGURATION);
    if (!isCallingPerm) {
        LOGE("permission verification failed");
        funcResult = PERMISSION_ERR;
        return SUCCEEDED;
    }
    if (!fontWeightScale.empty()) {
        funcResult = OnSetFontWeightScale(GetCallingUserId(), fontWeightScale);
        return SUCCEEDED;
    } else {
        LOGE("current fontWeightScale is empty!");
    }
    funcResult = SYS_ERR;
    return SUCCEEDED;
}

ErrCode UiAppearanceAbility::GetFontWeightScale(std::string& fontWeightScale, int32_t& funcResult)
{
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        auto it = usersParam_.find(GetCallingUserId());
        if (it != usersParam_.end()) {
            fontWeightScale = it->second.fontWeightScale;
        } else {
            fontWeightScale = BASE_SCALE;
        }
    }

    LOGD("get font weight scale :%{public}s", fontWeightScale.c_str());
    funcResult = SUCCEEDED;
    return SUCCEEDED;
}

void UiAppearanceAbility::UpdateSmartGestureModeCallback(bool isAutoMode, int32_t userId)
{
    bool ret = false;
    AppExecFwk::Configuration config;
    if (isAutoMode) {
        ret = config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_SMART_GESTURE_SWITCH,
            AppExecFwk::ConfigurationInner::SMART_GESTURE_AUTO);
    } else {
        ret = config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_SMART_GESTURE_SWITCH,
            AppExecFwk::ConfigurationInner::SMART_GESTURE_DISABLE);
    }
    if (!ret) {
        LOGE("AddItem failed, isAutoMode: %{public}d, userId: %{public}d", isAutoMode, userId);
        return;
    }

    UpdateConfiguration(config, userId);
}

void UiAppearanceAbility::UpdateDarkModeCallback(const bool isDarkMode, const int32_t userId)
{
    bool ret = false;
    std::string paramValue;
    AppExecFwk::Configuration config;
    if (isDarkMode) {
        ret = config.AddItem(
            AAFwk::GlobalConfigurationKey::SYSTEM_COLORMODE, AppExecFwk::ConfigurationInner::COLOR_MODE_DARK);
        paramValue.assign(DARK);
    } else {
        ret = config.AddItem(
            AAFwk::GlobalConfigurationKey::SYSTEM_COLORMODE, AppExecFwk::ConfigurationInner::COLOR_MODE_LIGHT);
        paramValue.assign(LIGHT);
    }
    if (!ret) {
        LOGE("AddItem failed, isDarkMode: %{public}d, userId: %{public}d", isDarkMode, userId);
        return;
    }

    std::vector<int32_t> effectiveUserIds = GetMultipleUsers();
    if (!UpdateConfiguration(config, userId, effectiveUserIds)) {
        return;
    }
    if (effectiveUserIds.size() > 1) {
        SetParameterWrap(PERSIST_DARKMODE_KEY, paramValue);
        for (const int32_t effectiveUserId : effectiveUserIds) {
            ConfigurePersistence(isDarkMode, effectiveUserId, paramValue);
        }
        return;
    }

    SetParameterWrap(PERSIST_DARKMODE_KEY, paramValue);
    ConfigurePersistence(isDarkMode, userId, paramValue);
}

std::vector<std::int32_t> UiAppearanceAbility::GetMultipleUsers()
{
    std::vector<int32_t> effectiveUserIds;
    std::vector<AccountSA::ForegroundOsAccount> accounts;
    auto result = AccountSA::OsAccountManager::GetForegroundOsAccounts(accounts);
    if (result == ERR_OK) {
        for (const auto &account : accounts) {
            effectiveUserIds.push_back(account.localId);
        }
    }
    return effectiveUserIds;
}

void UiAppearanceAbility::ConfigurePersistence(
    const bool isDarkMode, const int32_t userId, const std::string& paramValue)
{
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[userId].darkMode = isDarkMode ? ALWAYS_DARK : ALWAYS_LIGHT;
    }

    if (!SetParameterWrap(DarkModeParamAssignUser(userId), paramValue)) {
        LOGE("set parameter failed");
    }
}

int32_t UiAppearanceAbility::ConfigurePersistence(const int32_t userId, DarkMode mode, const std::string& paramValue)
{
    DarkModeManager::GetInstance().DoSwitchTemporaryColorMode(userId, mode == ALWAYS_DARK ? true : false);
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[userId].darkMode = mode;
    }

    // persist to file: etc/para/ui_appearance.para
    auto isSetPara = SetParameterWrap(DarkModeParamAssignUser(userId), paramValue);
    if (!isSetPara) {
        LOGE("set parameter failed");
        return SYS_ERR;
    }
    DarkModeManager::GetInstance().NotifyDarkModeUpdate(userId, mode == ALWAYS_DARK);
    return SUCCEEDED;
}
} // namespace ArkUi::UiAppearance
} // namespace OHOS
