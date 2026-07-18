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
#include <utility>

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
#include "setting_data_manager.h"
#include "tokenid_kit.h"

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

#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
// Want parameter keys carried by COMMON_EVENT_OS_ACCOUNT_SUB_PROFILE_SWITCHED, published by account_os_account.
constexpr const char* SUB_PROFILE_USER_ID_KEY = "userId";
constexpr const char* SUB_PROFILE_TO_PROFILE_ID_KEY = "toSubProfileId";
#endif
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
    const std::function<void(const int32_t)>& userSwitchCallback,
    const std::function<void(const int32_t, const int32_t)>& subProfileSwitchCallback)
    : EventFwk::CommonEventSubscriber(subscriberInfo), userSwitchCallback_(userSwitchCallback),
      subProfileSwitchCallback_(subProfileSwitchCallback)
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
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_OS_ACCOUNT_SUB_PROFILE_SWITCHED) {
#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
        if (subProfileSwitchCallback_ != nullptr) {
            int32_t userId = want.GetIntParam(SUB_PROFILE_USER_ID_KEY, INVALID_USER_ID);
            int32_t subProfileId = want.GetIntParam(SUB_PROFILE_TO_PROFILE_ID_KEY, INVALID_SUB_PROFILE_ID);
            subProfileSwitchCallback_(userId, subProfileId);
        }
#endif
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
        auto context = AccountContextHelper::GetForegroundContext(userId);
        DarkModeManager &manager = DarkModeManager::GetInstance();
        manager.OnSwitchContext(context);
        bool isDarkMode = false;
        manager.LoadUserSettingData(context, true, isDarkMode, true);
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

    const auto contexts = AccountContextHelper::GetContextsByUserIds(GetUserIds());
    std::string darkMode = LIGHT;
    if (getOldParam(PERSIST_DARKMODE_KEY, darkMode)) {
        for (const auto& context : contexts) {
            if (isParamAllreadaySetted(DarkModeParamAssignUser(context))) {
                continue;
            }
            SetParameterWrap(DarkModeParamAssignUser(context), darkMode);
            LOGI("context:%{public}s set darkMode %{public}s",
                AccountContextHelper::ToString(context).c_str(), darkMode.c_str());
        }
    }
    std::string fontSize = BASE_SCALE;
    if (getOldParam(FONT_SCAL_FOR_USER0, fontSize)) {
        for (const auto& context : contexts) {
            if (isParamAllreadaySetted(FontScaleParamAssignUser(context))) {
                continue;
            }
            SetParameterWrap(FontScaleParamAssignUser(context), fontSize);
            LOGI("context:%{public}s set fontSize %{public}s",
                AccountContextHelper::ToString(context).c_str(), fontSize.c_str());
        }
    }
    std::string fontWeightSize = BASE_SCALE;
    if (getOldParam(FONT_Weight_SCAL_FOR_USER0, fontWeightSize)) {
        fontWeightSize = GetDefaultFontWeightScaleValue(fontWeightSize);
        for (const auto& context : contexts) {
            if (isParamAllreadaySetted(FontWeightScaleParamAssignUser(context))) {
                continue;
            }
            SetParameterWrap(FontWeightScaleParamAssignUser(context), fontWeightSize);
            LOGI("context:%{public}s set fontWeightSize %{public}s",
                AccountContextHelper::ToString(context).c_str(), fontWeightSize.c_str());
        }
    }
    SetParameterWrap(FIRST_INITIALIZATION, "0");
    isNeedDoCompatibleProcess_ = false;
}

void UiAppearanceAbility::DoInitProcess()
{
    LOGI("DoInitProcess");
    BackGroundAppColorSwitchSettings::GetInstance().Initialize();
    auto loadContextParam = [](const AccountContext& context, const std::string& contextKey,
                                const std::string& baseKey, const std::string& defaultValue) {
        std::string value;
        if (GetParameterWrap(contextKey, value, "") && !value.empty()) {
            return value;
        }
        value = defaultValue;
        // Backfill a new sub-profile key from the legacy userId key during the first load after upgrade.
        if (AccountContextHelper::IsSubProfileContext(context) && GetParameterWrap(baseKey, value, defaultValue)) {
            SetParameterWrap(contextKey, value);
        }
        return value;
    };
    const auto contexts = AccountContextHelper::GetContextsByUserIds(GetUserIds());
    for (const auto& context : contexts) {
        std::string darkValue = loadContextParam(context, DarkModeParamAssignUser(context),
            DarkModeParamAssignUser(context.userId), LIGHT);

        std::string fontSize = loadContextParam(context, FontScaleParamAssignUser(context),
            FontScaleParamAssignUser(context.userId), BASE_SCALE);

        std::string fontWeight = loadContextParam(context, FontWeightScaleParamAssignUser(context),
            FontWeightScaleParamAssignUser(context.userId), GetDefaultFontWeightScaleValue(BASE_SCALE));

        UiAppearanceParam tmpParam;
        tmpParam.darkMode = darkValue == DARK ? DarkMode::ALWAYS_DARK : DarkMode::ALWAYS_LIGHT;
        tmpParam.fontScale = fontSize;
        tmpParam.fontWeightScale = fontWeight;
        {
            std::lock_guard<std::mutex> guard(usersParamMutex_);
            usersParam_[context] = tmpParam;
        }
        LOGI("init context:%{public}s, darkMode:%{public}s, fontSize:%{public}s, fontWeight:%{public}s",
            AccountContextHelper::ToString(context).c_str(), darkValue.c_str(), fontSize.c_str(),
            fontWeight.c_str());
    }
    isInitializationFinished_ = true;
}

void UiAppearanceAbility::UpdateCurrentUserConfiguration(const AccountContext& context, const bool isForceUpdate)
{
    UiAppearanceParam tmpParam;
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        tmpParam = usersParam_[context];
    }
    AppExecFwk::Configuration config;
    config.AddItem(
        AAFwk::GlobalConfigurationKey::SYSTEM_COLORMODE, tmpParam.darkMode == DarkMode::ALWAYS_DARK ? DARK : LIGHT);
    config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_SIZE_SCALE, tmpParam.fontScale);
    config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_WEIGHT_SCALE, tmpParam.fontWeightScale);

    auto appManagerInstance = GetAppManagerInstance();
    if (!appManagerInstance) {
        LOGE("GetAppManagerInstance error context:%{public}s", AccountContextHelper::ToString(context).c_str());
        return;
    }

    {
        std::lock_guard<std::mutex> onceFlagGuard(userSwitchUpdateConfigurationOnceFlagMutex_);
        if (isForceUpdate ||
            userSwitchUpdateConfigurationOnceFlag_.find(context) == userSwitchUpdateConfigurationOnceFlag_.end()) {
            appManagerInstance->UpdateConfiguration(config, context.userId);
            LOGI("update context:%{public}s configuration:%{public}s",
                AccountContextHelper::ToString(context).c_str(), config.GetName().c_str());
            userSwitchUpdateConfigurationOnceFlag_.insert(context);
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
    AccountContextSwitchFunc(GetForegroundAccountContext(userId));
}

void UiAppearanceAbility::SwitchAppearanceContext(const AccountContext& context)
{
    LOGI("switch appearance context:%{public}s", AccountContextHelper::ToString(context).c_str());
    AccountContextSwitchFunc(context);
}

void UiAppearanceAbility::ApplyAppearanceContextToUser(
    const AccountContext& sourceContext, const AccountContext& targetContext)
{
    UiAppearanceParam sourceParam;
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        auto it = usersParam_.find(sourceContext);
        if (it == usersParam_.end()) {
            LOGW("source context:%{public}s param not found", AccountContextHelper::ToString(sourceContext).c_str());
            return;
        }
        sourceParam = it->second;
        usersParam_[targetContext] = sourceParam;
    }

    if (!SetParameterWrap(DarkModeParamAssignUser(targetContext),
        sourceParam.darkMode == DarkMode::ALWAYS_DARK ? DARK : LIGHT)) {
        LOGE("set dark mode parameter failed, context:%{public}s",
            AccountContextHelper::ToString(targetContext).c_str());
    }
    if (!SetParameterWrap(FontScaleParamAssignUser(targetContext), sourceParam.fontScale)) {
        LOGE("set font scale parameter failed, context:%{public}s",
            AccountContextHelper::ToString(targetContext).c_str());
    }
    if (!SetParameterWrap(FontWeightScaleParamAssignUser(targetContext), sourceParam.fontWeightScale)) {
        LOGE("set font weight scale parameter failed, context:%{public}s",
            AccountContextHelper::ToString(targetContext).c_str());
    }
    UpdateCurrentUserConfiguration(targetContext, true);
}

void UiAppearanceAbility::AccountContextSwitchFunc(const AccountContext& context)
{
    DarkModeManager& manager = DarkModeManager::GetInstance();
    manager.OnSwitchContext(context);
    bool isDarkMode = false;
    int32_t code = manager.LoadUserSettingData(context, false, isDarkMode, false);

    if (isNeedDoCompatibleProcess_) {
        DoCompatibleProcess();
    }
    if (!isInitializationFinished_) {
        DoInitProcess();
    }

    bool isForceUpdate = false;
    if (code == ERR_OK && manager.IsColorModeNormal(context)) {
        DarkMode darkMode = isDarkMode ? ALWAYS_DARK : ALWAYS_LIGHT;
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        if (usersParam_[context].darkMode != darkMode) {
            usersParam_[context].darkMode = darkMode;
            isForceUpdate = true;
        }
    }
    // Sub-profiles under the same OS account share the AppMgr userId dimension but have distinct
    // appearance. The per-context "once" dedup in UpdateCurrentUserConfiguration would otherwise
    // fall back to USER0 on repeat/back switches, leaving the user's apps on the wrong appearance.
    if (AccountContextHelper::IsSubProfileContext(context)) {
        isForceUpdate = true;
    }

    UpdateCurrentUserConfiguration(context, isForceUpdate);
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
#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_OS_ACCOUNT_SUB_PROFILE_SWITCHED);
#endif
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);

    uiAppearanceEventSubscriber_ = std::make_shared<UiAppearanceEventSubscriber>(subscribeInfo,
        [this](const int32_t userId) { UserSwitchFunc(userId); },
        [this](const int32_t userId, const int32_t subProfileId) { HandleSubProfileSwitched(userId, subProfileId); });
    bool subResult = EventFwk::CommonEventManager::SubscribeCommonEvent(uiAppearanceEventSubscriber_);
    if (!subResult) {
        LOGW("subscribe user switch event error");
    }
}

void UiAppearanceAbility::HandleSubProfileSwitched(int32_t userId, int32_t subProfileId)
{
#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
    if (userId != USER100) {
        if (userId <= INVALID_USER_ID) {
            LOGI("ignore invalid subProfile switched event, userId:%{public}d, subProfileId:%{public}d",
                userId, subProfileId);
            return;
        }
        AccountContext centerContext = GetForegroundAccountContext(USER100);
        AccountContext targetContext = subProfileId == INVALID_SUB_PROFILE_ID ?
            GetForegroundAccountContext(userId) : AccountContextHelper::CreateContext(userId, subProfileId);
        LOGI("apply center appearance on subProfile switch, target:%{public}s",
            AccountContextHelper::ToString(targetContext).c_str());
        ApplyAppearanceContextToUser(centerContext, targetContext);
        return;
    }

    AccountContext centerContext = subProfileId == INVALID_SUB_PROFILE_ID ?
        GetForegroundAccountContext(userId) : AccountContextHelper::CreateContext(userId, subProfileId);
    SwitchAppearanceContext(centerContext);

    std::vector<int32_t> effectiveUserIds = GetMultipleUsers();
    if (effectiveUserIds.empty()) {
        return;
    }

    for (const int32_t effectiveUserId : effectiveUserIds) {
        if (effectiveUserId == USER100) {
            continue;
        }
        ApplyAppearanceContextToUser(centerContext, GetForegroundAccountContext(effectiveUserId));
    }
#endif
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
        UpdateCurrentUserConfiguration(GetForegroundAccountContext(userId), false);
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

AccountContext UiAppearanceAbility::GetCallingAccountContext()
{
    return GetForegroundAccountContext(GetCallingUserId());
}

AccountContext UiAppearanceAbility::GetForegroundAccountContext(int32_t fallbackUserId)
{
    return AccountContextHelper::GetForegroundContext(fallbackUserId);
}

std::string UiAppearanceAbility::DarkModeParamAssignUser(const int32_t userId)
{
    return DarkModeParamAssignUser(AccountContextHelper::CreateBaseContext(userId));
}
std::string UiAppearanceAbility::FontScaleParamAssignUser(const int32_t userId)
{
    return FontScaleParamAssignUser(AccountContextHelper::CreateBaseContext(userId));
}
std::string UiAppearanceAbility::FontWeightScaleParamAssignUser(const int32_t userId)
{
    return FontWeightScaleParamAssignUser(AccountContextHelper::CreateBaseContext(userId));
}

std::string UiAppearanceAbility::DarkModeParamAssignUser(const AccountContext& context)
{
    return AccountContextHelper::BuildUserParamKey(PERSIST_DARKMODE_KEY_FOR_NONE, context);
}
std::string UiAppearanceAbility::FontScaleParamAssignUser(const AccountContext& context)
{
    return AccountContextHelper::BuildUserParamKey(FONT_SCAL_FOR_NONE, context);
}
std::string UiAppearanceAbility::FontWeightScaleParamAssignUser(const AccountContext& context)
{
    return AccountContextHelper::BuildUserParamKey(FONT_WEIGHT_SCAL_FOR_NONE, context);
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

int32_t UiAppearanceAbility::OnSetDarkMode(const AccountContext& context, DarkMode mode)
{
    LOGI("setDarkMode, context:%{public}s, mode: %{public}d",
        AccountContextHelper::ToString(context).c_str(), mode);
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
    if (!UpdateConfiguration(config, context.userId, effectiveUserIds)) {
        return SYS_ERR;
    }

    if (effectiveUserIds.size() > 1) {
        SetParameterWrap(PERSIST_DARKMODE_KEY, paramValue);
        for (const int32_t effectiveUserId : effectiveUserIds) {
            if (ConfigurePersistence(GetForegroundAccountContext(effectiveUserId), mode, paramValue) != SUCCEEDED) {
                return SYS_ERR;
            }
        }
        return SUCCEEDED;
    }

    SetParameterWrap(PERSIST_DARKMODE_KEY, paramValue);
    return ConfigurePersistence(context, mode, paramValue);
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

    auto context = GetCallingAccountContext();
    DarkMode currentDarkMode = DarkMode::ALWAYS_LIGHT;
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        auto it = usersParam_.find(context);
        if (it != usersParam_.end()) {
            currentDarkMode = it->second.darkMode;
        }
    }
    if (darkMode != currentDarkMode) {
        funcResult = OnSetDarkMode(context, darkMode);
        return SUCCEEDED;
    } else {
        LOGW("current color mode is %{public}d, no need to change", darkMode);
        funcResult = SYS_ERR;
        return SUCCEEDED;
    }
}

DarkMode UiAppearanceAbility::InitGetDarkMode(const AccountContext& context)
{
    std::string valueGet = LIGHT;

    // LIGHT is the default.
    auto res = GetParameterWrap(DarkModeParamAssignUser(context), valueGet);
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
        auto it = usersParam_.find(GetCallingAccountContext());
        if (it != usersParam_.end()) {
            funcResult = it->second.darkMode;
            return SUCCEEDED;
        }
    }

    funcResult = DarkMode::ALWAYS_LIGHT;
    return SUCCEEDED;
}

int32_t UiAppearanceAbility::OnSetFontScale(const AccountContext& context, const std::string& fontScale)
{
    bool ret = false;
    AppExecFwk::Configuration config;
    ret = config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_SIZE_SCALE, fontScale);
    if (!ret) {
        LOGE("AddItem failed, fontScale = %{public}s", fontScale.c_str());
        return INVALID_ARG;
    }

#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
    std::vector<int32_t> effectiveUserIds = GetMultipleUsers();
    if (effectiveUserIds.size() > 1) {
        if (!UpdateConfiguration(config, context.userId, effectiveUserIds)) {
            return SYS_ERR;
        }
        SetParameterWrap(FONT_SCAL_FOR_USER0, fontScale);
        for (const int32_t effectiveUserId : effectiveUserIds) {
            if (ConfigureFontScalePersistence(GetForegroundAccountContext(effectiveUserId), fontScale) != SUCCEEDED) {
                return SYS_ERR;
            }
        }
        return SUCCEEDED;
    }
#endif

    if (!UpdateConfiguration(config, context.userId)) {
        return SYS_ERR;
    }

    SetParameterWrap(FONT_SCAL_FOR_USER0, fontScale);
    return ConfigureFontScalePersistence(context, fontScale);
}

int32_t UiAppearanceAbility::ConfigureFontScalePersistence(const AccountContext& context, const std::string& fontScale)
{
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[context].fontScale = fontScale;
    }

    // persist to file: etc/para/ui_appearance.para
    auto isSetPara = SetParameterWrap(FontScaleParamAssignUser(context), fontScale);
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
        funcResult = OnSetFontScale(GetCallingAccountContext(), fontScale);
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
        auto it = usersParam_.find(GetCallingAccountContext());
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

int32_t UiAppearanceAbility::OnSetFontWeightScale(const AccountContext& context, const std::string& fontWeightScale)
{
    bool ret = false;
    AppExecFwk::Configuration config;
    ret = config.AddItem(AAFwk::GlobalConfigurationKey::SYSTEM_FONT_WEIGHT_SCALE, fontWeightScale);
    if (!ret) {
        LOGE("AddItem failed, fontWeightScale = %{public}s", fontWeightScale.c_str());
        return INVALID_ARG;
    }

#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
    std::vector<int32_t> effectiveUserIds = GetMultipleUsers();
    if (effectiveUserIds.size() > 1) {
        if (!UpdateConfiguration(config, context.userId, effectiveUserIds)) {
            return SYS_ERR;
        }
        SetParameterWrap(FONT_Weight_SCAL_FOR_USER0, fontWeightScale);
        for (const int32_t effectiveUserId : effectiveUserIds) {
            if (ConfigureFontWeightScalePersistence(
                GetForegroundAccountContext(effectiveUserId), fontWeightScale) != SUCCEEDED) {
                return SYS_ERR;
            }
        }
        return SUCCEEDED;
    }
#endif

    if (!UpdateConfiguration(config, context.userId)) {
        return SYS_ERR;
    }

    SetParameterWrap(FONT_Weight_SCAL_FOR_USER0, fontWeightScale);
    return ConfigureFontWeightScalePersistence(context, fontWeightScale);
}

int32_t UiAppearanceAbility::ConfigureFontWeightScalePersistence(
    const AccountContext& context, const std::string& fontWeightScale)
{
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[context].fontWeightScale = fontWeightScale;
    }

    // persist to file: etc/para/ui_appearance.para
    auto isSetPara = SetParameterWrap(FontWeightScaleParamAssignUser(context), fontWeightScale);
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
        funcResult = OnSetFontWeightScale(GetCallingAccountContext(), fontWeightScale);
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
        auto it = usersParam_.find(GetCallingAccountContext());
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

ErrCode UiAppearanceAbility::SetSettingData(const std::string& key, const std::string& value, int32_t& funcResult)
{
    auto selfToken = IPCSkeleton::GetCallingFullTokenID();
    if (!Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(selfToken)) {
        return UiAppearanceAbilityErrCode::NOT_SYSTEM_APP;
    }
    SettingDataManager& manager = SettingDataManager::GetInstance();
    std::lock_guard lock(settingMutex_);
    AccountContext context = GetCallingAccountContext();
    manager.SetStringValue(AccountContextHelper::BuildSettingKey(key, context), value, context.userId);

    LOGD("set setting data key:%{public}s value:%{public}s", key.c_str(), value.c_str());
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
    AccountContext context = GetForegroundAccountContext(userId);
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
            ConfigurePersistence(isDarkMode, GetForegroundAccountContext(effectiveUserId), paramValue);
        }
        return;
    }

    SetParameterWrap(PERSIST_DARKMODE_KEY, paramValue);
    ConfigurePersistence(isDarkMode, context, paramValue);
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
    const bool isDarkMode, const AccountContext& context, const std::string& paramValue)
{
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[context].darkMode = isDarkMode ? ALWAYS_DARK : ALWAYS_LIGHT;
    }

    if (!SetParameterWrap(DarkModeParamAssignUser(context), paramValue)) {
        LOGE("set parameter failed");
    }
}

int32_t UiAppearanceAbility::ConfigurePersistence(
    const AccountContext& context, DarkMode mode, const std::string& paramValue)
{
    DarkModeManager::GetInstance().DoSwitchTemporaryColorMode(context, mode == ALWAYS_DARK ? true : false);
    {
        std::lock_guard<std::mutex> guard(usersParamMutex_);
        usersParam_[context].darkMode = mode;
    }

    // persist to file: etc/para/ui_appearance.para
    auto isSetPara = SetParameterWrap(DarkModeParamAssignUser(context), paramValue);
    if (!isSetPara) {
        LOGE("set parameter failed");
        return SYS_ERR;
    }
    DarkModeManager::GetInstance().NotifyDarkModeUpdate(context, mode == ALWAYS_DARK);
    return SUCCEEDED;
}
} // namespace ArkUi::UiAppearance
} // namespace OHOS
