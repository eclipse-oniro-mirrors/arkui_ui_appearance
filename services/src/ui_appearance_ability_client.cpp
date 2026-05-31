/*
 * Copyright (C) 2022-2025 Huawei Device Co., Ltd.
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

#include "ui_appearance_ability_client.h"

#include <string>
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "ui_appearance_ability_proxy.h"
#include "ui_appearance_log.h"
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"

namespace OHOS {
namespace ArkUi::UiAppearance {
sptr<UiAppearanceAbilityClient> UiAppearanceAbilityClient::GetInstance()
{
    static sptr<UiAppearanceAbilityClient> instance = new UiAppearanceAbilityClient;
    return instance;
}

UiAppearanceAbilityClient::UiAppearanceAbilityClient()
{
    GetUiAppearanceServiceProxy();
}

sptr<IUiAppearanceAbility> UiAppearanceAbilityClient::GetUiAppearanceServiceProxy()
{
    std::lock_guard guard(serviceProxyLock_);
    if (uiAppearanceServiceProxy_ == nullptr) {
        LOGE("Redo CreateUiAppearanceServiceProxy");
        uiAppearanceServiceProxy_ = CreateUiAppearanceServiceProxy();
    }
    return uiAppearanceServiceProxy_;
}

int32_t UiAppearanceAbilityClient::SetDarkMode(DarkMode mode)
{
    if (!GetUiAppearanceServiceProxy()) {
        LOGE("SetDarkMode quit because redoing CreateUiAppearanceServiceProxy failed.");
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    int32_t funcRes = -1;
    auto res = GetUiAppearanceServiceProxy()->SetDarkMode(mode, funcRes);
    if (res != ERR_OK) {
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    return funcRes;
}

int32_t UiAppearanceAbilityClient::GetDarkMode()
{
    if (!GetUiAppearanceServiceProxy()) {
        LOGE("GetDarkMode quit because redoing CreateUiAppearanceServiceProxy failed.");
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    int32_t funcRes = -1;
    auto res = GetUiAppearanceServiceProxy()->GetDarkMode(funcRes);
    if (res != ERR_OK) {
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    return funcRes;
}

int32_t UiAppearanceAbilityClient::SetFontScale(std::string &fontScale)
{
    if (!GetUiAppearanceServiceProxy()) {
        LOGE("SetDarkMode quit because redoing CreateUiAppearanceServiceProxy failed.");
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    int32_t funcRes = -1;
    auto res = GetUiAppearanceServiceProxy()->SetFontScale(fontScale, funcRes);
    if (res != ERR_OK) {
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    return funcRes;
}

int32_t UiAppearanceAbilityClient::GetFontScale(std::string &fontScale)
{
    if (!GetUiAppearanceServiceProxy()) {
        LOGE("GetDarkMode quit because redoing CreateUiAppearanceServiceProxy failed.");
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    int id = HiviewDFX::XCollie::GetInstance().SetTimer(
        "GetFontScale", 10, nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
    int32_t funcRes = -1;
    auto res = GetUiAppearanceServiceProxy()->GetFontScale(fontScale, funcRes);
    HiviewDFX::XCollie::GetInstance().CancelTimer(id);
    if (res != ERR_OK) {
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    return funcRes;
}

int32_t UiAppearanceAbilityClient::SetFontWeightScale(std::string &fontWeightScale)
{
    if (!GetUiAppearanceServiceProxy()) {
        LOGE("SetDarkMode quit because redoing CreateUiAppearanceServiceProxy failed.");
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    int32_t funcRes = -1;
    auto res = GetUiAppearanceServiceProxy()->SetFontWeightScale(fontWeightScale, funcRes);
    if (res != ERR_OK) {
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    return funcRes;
}

int32_t UiAppearanceAbilityClient::GetFontWeightScale(std::string &fontWeightScale)
{
    if (!GetUiAppearanceServiceProxy()) {
        LOGE("GetDarkMode quit because redoing CreateUiAppearanceServiceProxy failed.");
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    int32_t funcRes = -1;
    auto res = GetUiAppearanceServiceProxy()->GetFontWeightScale(fontWeightScale, funcRes);
    if (res != ERR_OK) {
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    return funcRes;
}

int32_t UiAppearanceAbilityClient::SetSettingData(std::string key, std::string value)
{
    if (!GetUiAppearanceServiceProxy()) {
        LOGE("SetSettingData quit because redoing CreateUiAppearanceServiceProxy failed.");
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    auto selfToken = IPCSkeleton::GetSelfTokenID();
    if (!Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(selfToken)) {
        LOGE("SetSettingData quit because caller is not system app.");
        return UiAppearanceAbilityErrCode::NOT_SYSTEM_APP;
    }
    int32_t funcRes = -1;
    auto res = GetUiAppearanceServiceProxy()->SetSettingData(key, value, funcRes);
    if (res != ERR_OK) {
        return UiAppearanceAbilityErrCode::SYS_ERR;
    }
    return funcRes;
}


sptr<IUiAppearanceAbility> UiAppearanceAbilityClient::CreateUiAppearanceServiceProxy()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        LOGE("Get SystemAbilityManager failed.");
        return nullptr;
    }

    sptr<IRemoteObject> systemAbility = systemAbilityManager->GetSystemAbility(ARKUI_UI_APPEARANCE_SERVICE_ID);
    if (systemAbility == nullptr) {
        LOGE("Get SystemAbility failed.");
        return nullptr;
    }

    sptr<UiAppearanceDeathRecipient> deathRecipient_ = new UiAppearanceDeathRecipient;
    systemAbility->AddDeathRecipient(deathRecipient_);
    sptr<IUiAppearanceAbility> uiAppearanceServiceProxy =
        iface_cast<IUiAppearanceAbility>(systemAbility);
    if (uiAppearanceServiceProxy == nullptr) {
        LOGE("Get uiAppearanceServiceProxy from SA failed.");
        return nullptr;
    }
    LOGI("Get uiAppearanceServiceProxy successful.");
    return uiAppearanceServiceProxy;
}

void UiAppearanceAbilityClient::OnRemoteSaDied(const wptr<IRemoteObject>& remote)
{
    // Used for new connections after the service may be disconnected.
    std::lock_guard guard(serviceProxyLock_);
    uiAppearanceServiceProxy_ = CreateUiAppearanceServiceProxy();
}

void UiAppearanceDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& object)
{
    LOGI("UiAppearanceDeathRecipient on remote systemAbility died.");
    UiAppearanceAbilityClient::GetInstance()->OnRemoteSaDied(object);
}
} // namespace ArkUi::UiAppearance
} // namespace OHOS
