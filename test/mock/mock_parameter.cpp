/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <securec.h>

#include <string>
#include <unordered_map>

#include "syspara/parameter.h"

namespace {
constexpr int32_t buffSize = 10; // buff len: 64
char value_[buffSize] = "light";
std::unordered_map<std::string, std::string> valuesByKey_;
bool getParameterShouldFail_ = false;
} // namespace

namespace OHOS::ArkUi::UiAppearance {
void MockSetParameterValue(const std::string& key, const std::string& value)
{
    valuesByKey_[key] = value;
}

void MockClearParameterValues()
{
    valuesByKey_.clear();
    getParameterShouldFail_ = false;
}

void MockSetGetParameterShouldFail(bool shouldFail)
{
    getParameterShouldFail_ = shouldFail;
}
} // namespace OHOS::ArkUi::UiAppearance

int GetParameter(const char* key, const char* def, char* value, unsigned int len)
{
    if (getParameterShouldFail_) {
        return -1;
    }
    auto it = valuesByKey_.find(key);
    if (it != valuesByKey_.end()) {
        (void)strncpy_s(value, len, it->second.c_str(), it->second.size() + 1);
        return 1;
    }
    (void)strncpy_s(value, len, def, strlen(def) + 1);
    return 1;
}

int SetParameter(const char* key, const char* value)
{
    (void)strncpy_s(value_, buffSize, value, strlen(value) + 1);
    valuesByKey_[key] = value;
    return 0;
}
