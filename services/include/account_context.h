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

#ifndef UI_APPEARANCE_ACCOUNT_CONTEXT_H
#define UI_APPEARANCE_ACCOUNT_CONTEXT_H

#include <cstdint>
#include <list>
#include <string>
#include <vector>

namespace OHOS::ArkUi::UiAppearance {
constexpr int32_t INVALID_SUB_PROFILE_ID = -1;

struct AccountContext {
    AccountContext(int32_t user = -1, int32_t subProfile = INVALID_SUB_PROFILE_ID)
        : userId(user), subProfileId(subProfile)
    {}

    int32_t userId;
    int32_t subProfileId;

    bool operator<(const AccountContext& other) const
    {
        if (userId != other.userId) {
            return userId < other.userId;
        }
        return subProfileId < other.subProfileId;
    }

    bool operator==(const AccountContext& other) const
    {
        return userId == other.userId && subProfileId == other.subProfileId;
    }
};

class AccountContextHelper {
public:
    AccountContextHelper() = delete;
    ~AccountContextHelper() = delete;

    static AccountContext CreateBaseContext(int32_t userId);
    static AccountContext CreateContext(int32_t userId, int32_t subProfileId);
    static AccountContext GetForegroundContext(int32_t fallbackUserId);
    static std::vector<AccountContext> GetContextsByUserId(int32_t userId);
    static std::vector<AccountContext> GetContextsByUserIds(const std::list<int32_t>& userIds);
    static bool IsSubProfileContext(const AccountContext& context);
    static std::string ToString(const AccountContext& context);
    static std::string BuildUserParamKey(const std::string& prefix, const AccountContext& context);
    static std::string BuildSettingKey(const std::string& key, const AccountContext& context);
    static uint64_t BuildTimerKey(const AccountContext& context);
};
} // namespace OHOS::ArkUi::UiAppearance

#endif // UI_APPEARANCE_ACCOUNT_CONTEXT_H
