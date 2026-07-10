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

#include "account_context.h"

#include "errors.h"
#include "ui_appearance_log.h"

#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
#include "os_account_subprofile_client.h"
#endif

namespace OHOS::ArkUi::UiAppearance {
namespace {
bool GetForegroundSubProfileId(int32_t userId, int32_t& subProfileId)
{
    subProfileId = INVALID_SUB_PROFILE_ID;
#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
    auto errCode = AccountSA::OsAccountSubProfileClient::GetInstance().GetOsAccountForegroundSubProfileId(
        userId, subProfileId);
    if (errCode != ERR_OK || subProfileId == INVALID_SUB_PROFILE_ID) {
        LOGW("Get foreground subProfileId failed, userId:%{public}d, err:%{public}d, subProfileId:%{public}d",
            userId, errCode, subProfileId);
        subProfileId = INVALID_SUB_PROFILE_ID;
        return false;
    }
    return true;
#else
    return false;
#endif
}
} // namespace

AccountContext AccountContextHelper::CreateBaseContext(int32_t userId)
{
    return { userId, INVALID_SUB_PROFILE_ID };
}

AccountContext AccountContextHelper::CreateContext(int32_t userId, int32_t subProfileId)
{
#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
    if (subProfileId == INVALID_SUB_PROFILE_ID) {
        return CreateBaseContext(userId);
    }
    return { userId, subProfileId };
#else
    return CreateBaseContext(userId);
#endif
}

AccountContext AccountContextHelper::GetForegroundContext(int32_t fallbackUserId)
{
    int32_t subProfileId = INVALID_SUB_PROFILE_ID;
    if (GetForegroundSubProfileId(fallbackUserId, subProfileId)) {
        return { fallbackUserId, subProfileId };
    }
    return CreateBaseContext(fallbackUserId);
}

std::vector<AccountContext> AccountContextHelper::GetContextsByUserId(int32_t userId)
{
#ifdef ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE
    std::vector<int32_t> subProfileIds;
    auto errCode = AccountSA::OsAccountSubProfileClient::GetInstance().GetOsAccountSubProfileIds(
        userId, subProfileIds);
    if (errCode != ERR_OK || subProfileIds.empty()) {
        LOGW("Get subProfileIds failed, userId:%{public}d, err:%{public}d", userId, errCode);
        return { CreateBaseContext(userId) };
    }
    std::vector<AccountContext> contexts;
    for (auto subProfileId : subProfileIds) {
        if (subProfileId == INVALID_SUB_PROFILE_ID) {
            continue;
        }
        contexts.emplace_back(AccountContext { userId, subProfileId });
    }
    if (contexts.empty()) {
        contexts.emplace_back(CreateBaseContext(userId));
    }
    return contexts;
#else
    return { CreateBaseContext(userId) };
#endif
}

std::vector<AccountContext> AccountContextHelper::GetContextsByUserIds(const std::list<int32_t>& userIds)
{
    std::vector<AccountContext> contexts;
    for (auto userId : userIds) {
        auto userContexts = GetContextsByUserId(userId);
        contexts.insert(contexts.end(), userContexts.begin(), userContexts.end());
    }
    return contexts;
}

bool AccountContextHelper::IsSubProfileContext(const AccountContext& context)
{
    return context.subProfileId != INVALID_SUB_PROFILE_ID;
}

std::string AccountContextHelper::ToString(const AccountContext& context)
{
    if (!IsSubProfileContext(context)) {
        return std::to_string(context.userId);
    }
    return std::to_string(context.userId) + "." + std::to_string(context.subProfileId);
}

std::string AccountContextHelper::BuildUserParamKey(const std::string& prefix, const AccountContext& context)
{
    return prefix + ToString(context);
}

std::string AccountContextHelper::BuildSettingKey(const std::string& key, const AccountContext& context)
{
    if (!IsSubProfileContext(context)) {
        return key;
    }
    return key + "." + std::to_string(context.subProfileId);
}

uint64_t AccountContextHelper::BuildTimerKey(const AccountContext& context)
{
    if (!IsSubProfileContext(context)) {
        return static_cast<uint64_t>(context.userId);
    }
    // Use the high 32 bits for userId and the low 32 bits for subProfileId.
    return (static_cast<uint64_t>(static_cast<uint32_t>(context.userId)) << 32) |
        static_cast<uint32_t>(context.subProfileId);
}
} // namespace OHOS::ArkUi::UiAppearance
