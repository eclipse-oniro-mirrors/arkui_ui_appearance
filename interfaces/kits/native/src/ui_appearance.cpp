/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ui_appearance.h"

#include "ui_appearance_ability_client.h"

namespace OHOS {
namespace ArkUi {
namespace UiAppearance {
UIAppearance::SetDarkModeFunc UIAppearance::setDarkModeFunc_ = [](DarkMode mode) {
    return static_cast<UiAppearanceAbilityErrCode>(UiAppearanceAbilityClient::GetInstance()->SetDarkMode(mode));
};

UIAppearance::GetDarkModeFunc UIAppearance::getDarkModeFunc_ = [](DarkMode& mode) {
    int32_t result = UiAppearanceAbilityClient::GetInstance()->GetDarkMode();
    if (result == static_cast<int32_t>(DarkMode::ALWAYS_DARK) ||
        result == static_cast<int32_t>(DarkMode::ALWAYS_LIGHT) || result == static_cast<int32_t>(DarkMode::UNKNOWN)) {
        mode = static_cast<DarkMode>(result);
        return UiAppearanceAbilityErrCode::SUCCEEDED;
    }
    return static_cast<UiAppearanceAbilityErrCode>(result);
};

UIAppearance::SetSettingDataFunc UIAppearance::setSettingDataFunc_ = [](std::string key, std::string value) {
    return static_cast<UiAppearanceAbilityErrCode>(
        UiAppearanceAbilityClient::GetInstance()->SetSettingData(key, value));
};

UiAppearanceAbilityErrCode UIAppearance::SetDarkMode(DarkMode mode)
{
    return setDarkModeFunc_(mode);
}

UiAppearanceAbilityErrCode UIAppearance::GetDarkMode(DarkMode& mode)
{
    return getDarkModeFunc_(mode);
}

UiAppearanceAbilityErrCode UIAppearance::SetSettingData(std::string key, std::string value)
{
    return setSettingDataFunc_(key, value);
}

extern "C" __attribute__((visibility("default"))) int32_t OH_UIAppearance_SetSettingDate(
    const char* key, const char* value)
{
    return UIAppearance::SetSettingData(std::string(key), std::string(value));
}
} // namespace UiAppearance
} // namespace ArkUi
} // namespace OHOS
