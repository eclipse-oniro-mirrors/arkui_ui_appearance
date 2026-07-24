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

#ifndef UI_APPEARANCE_ABILITY_H
#define UI_APPEARANCE_ABILITY_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "account_context.h"
#include "appmgr/app_mgr_proxy.h"
#include "common_event_manager.h"
#include "system_ability.h"
#include "ui_appearance_types.h"
#include "ui_appearance_ability_stub.h"

namespace OHOS {
namespace ArkUi::UiAppearance {
class UiAppearanceEventSubscriber : public EventFwk::CommonEventSubscriber {
public:
    UiAppearanceEventSubscriber(const EventFwk::CommonEventSubscribeInfo& subscriberInfo,
        const std::function<void(const int32_t)>& userSwitchCallback,
        const std::function<void(const int32_t, const int32_t)>& subProfileSwitchCallback);
    ~UiAppearanceEventSubscriber() override = default;
    void OnReceiveEvent(const EventFwk::CommonEventData& data) override;

    static void TimeChangeCallback();

    void BootCompetedCallback();

private:
    std::function<void(const int32_t)> userSwitchCallback_;
    std::function<void(const int32_t, const int32_t)> subProfileSwitchCallback_;
    std::once_flag bootCompleteFlag_;
};

class UiAppearanceAbility : public SystemAbility, public UiAppearanceAbilityStub {
    DECLARE_SYSTEM_ABILITY(UiAppearanceAbility);

public:
    struct UiAppearanceParam {
        UiAppearanceParam();
        DarkMode darkMode = DarkMode::ALWAYS_LIGHT;
        std::string fontScale = "1";
        std::string fontWeightScale = "1";
    };
    UiAppearanceAbility(int32_t saId, bool runOnCreate);
    ~UiAppearanceAbility() = default;

    ErrCode SetDarkMode(int32_t mode, int32_t& funcResult) override;
    ErrCode GetDarkMode(int32_t& funcResult) override;
    ErrCode GetFontScale(std::string& fontScale, int32_t& funcResult) override;
    ErrCode SetFontScale(const std::string& fontScale, int32_t& funcResult) override;
    ErrCode GetFontWeightScale(std::string& fontWeightScale, int32_t& funcResult) override;
    ErrCode SetFontWeightScale(const std::string& fontWeightScale, int32_t& funcResult) override;
    ErrCode SetSettingData(const std::string& key, const std::string& value, int32_t& funcResult) override;

protected:
    void OnStart() override;
    void OnStop() override;

    void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;

private:
    sptr<AppExecFwk::IAppMgr> GetAppManagerInstance();
    bool VerifyAccessToken(const std::string& permissionName);
    void Init();
    void SubscribeCommonEvent();
    void HandleSubProfileSwitched(int32_t userId, int32_t subProfileId);
    bool UpdateConfiguration(const AppExecFwk::Configuration& configuration, const int32_t userId,
        const std::vector<std::int32_t>& effectiveUserIds = {});
    void DoCompatibleProcess();
    int32_t GetCallingUserId();
    AccountContext GetCallingAccountContext();
    AccountContext GetForegroundAccountContext(int32_t fallbackUserId);
    std::list<int32_t> GetUserIds();
    void UserSwitchFunc(const int32_t userId);
    void SwitchAppearanceContext(const AccountContext& context);
    void ApplyAppearanceContextToUser(const AccountContext& sourceContext, const AccountContext& targetContext);
    void AccountContextSwitchFunc(const AccountContext& context);
    void DoInitProcess();

    void UpdateCurrentUserConfiguration(const AccountContext& context, const bool isForceUpdate);
    int32_t OnSetDarkMode(const AccountContext& context, DarkMode mode);
    DarkMode InitGetDarkMode(const AccountContext& context);
    int32_t OnSetFontScale(const AccountContext& context, const std::string& fontScale);
    int32_t OnSetFontWeightScale(const AccountContext& context, const std::string& fontWeightScale);
    int32_t ConfigureFontScalePersistence(const AccountContext& context, const std::string& fontScale);
    int32_t ConfigureFontWeightScalePersistence(const AccountContext& context, const std::string& fontWeightScale);
    std::string DarkNodeConfigurationAssignUser(const int32_t userId);
    std::string FontScaleConfigurationAssignUser(const int32_t userId);
    std::string FontWeightScaleConfigurationAssignUser(const int32_t userId);
    std::string DarkModeParamAssignUser(const AccountContext& context);
    std::string FontScaleParamAssignUser(const AccountContext& context);
    std::string FontWeightScaleParamAssignUser(const AccountContext& context);
    std::string DarkModeParamAssignUser(const int32_t userId);
    std::string FontScaleParamAssignUser(const int32_t userId);
    std::string FontWeightScaleParamAssignUser(const int32_t userId);

    void UpdateSmartGestureModeCallback(bool isAutoMode, int32_t userId);
    void UpdateDarkModeCallback(bool isDarkMode, int32_t userId);
    bool BackGroundAppColorSwitch(sptr<AppExecFwk::IAppMgr> appManagerInstance, const int32_t userId);
    std::vector<std::int32_t> GetMultipleUsers();
    void ConfigurePersistence(const bool isDarkMode, const AccountContext& context, const std::string& paramValue);
    int32_t ConfigurePersistence(const AccountContext& context, DarkMode mode, const std::string& paramValue);

    std::shared_ptr<UiAppearanceEventSubscriber> uiAppearanceEventSubscriber_;
    std::mutex usersParamMutex_;
    std::map<AccountContext, UiAppearanceParam> usersParam_;
    std::atomic<bool> isNeedDoCompatibleProcess_ = false;
    std::atomic<bool> isInitializationFinished_ = false;
    std::set<AccountContext> userSwitchUpdateConfigurationOnceFlag_;
    std::mutex userSwitchUpdateConfigurationOnceFlagMutex_;
    std::mutex settingMutex_;
};
} // namespace ArkUi::UiAppearance
} // namespace OHOS
#endif // UI_APPEARANCE_ABILITY_H
