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

#include <chrono>
#include <cstdio>
#include <gtest/gtest.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#include "accesstoken_kit.h"
#include "syspara/parameter.h"
#include "system_ability_definition.h"
#define private public
#define protected public
#include "ui_appearance_ability.h"
#undef private
#undef protected
#include "ui_appearance_log.h"
#include "alarm_timer_manager.h"

using namespace testing::ext;
static constexpr int UISERVER_UID = 3050;

namespace OHOS {
namespace ArkUi::UiAppearance {

void MockSetParameterValue(const std::string& key, const std::string& value);
void MockClearParameterValues();
void MockSetGetParameterShouldFail(bool shouldFail);

const int DAY_TO_SECOND = 24 * 60 * 60;
const int DAY_TO_MINUTE = 24 * 60;
const int SECOND_TO_MILLI = 1000;
const int MINUTE_TO_SECOND = 60;
static const std::string STANDARD_FONT_WEIGHT = "const.standard_font_weight";

class UiAppearanceAbilityTest : public UiAppearanceAbility {
public:
    UiAppearanceAbilityTest() : UiAppearanceAbility(ARKUI_UI_APPEARANCE_SERVICE_ID, true) {}
    ~UiAppearanceAbilityTest() {}
    void OnStart() override
    {
        return;
    }
};

class DarkModeTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static sptr<UiAppearanceAbilityTest> GetUiAppearanceAbilityTest()
    {
        return new UiAppearanceAbilityTest;
    }

    static std::shared_ptr<AlarmTimerManager> GetAlarmTimerManager()
    {
        return std::make_shared<AlarmTimerManager>();
    }

private:
    int userId_;
};

void DarkModeTest::SetUpTestCase(void) {}

void DarkModeTest::TearDownTestCase(void) {}

void DarkModeTest::SetUp(void)
{
    userId_ = geteuid();
    seteuid(UISERVER_UID);
    MockClearParameterValues();
}

void DarkModeTest::TearDown(void)
{
    MockClearParameterValues();
    seteuid(userId_);
}

/**
 * @tc.name: ui_appearance_test_001
 * @tc.desc: Test SetDarkMode and GetDarkMode APIs when setting dark/light.
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_001, TestSize.Level0)
{
    LOGI("Test SetDarkMode and GetDarkMode APIs when setting dark/light.");

    auto test = DarkModeTest::GetUiAppearanceAbilityTest();
    int32_t result = -1;
    test->SetDarkMode(DarkMode::ALWAYS_DARK, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    int32_t mode = -1;
    test->GetDarkMode(mode);
    EXPECT_EQ(mode, DarkMode::ALWAYS_DARK);

    test->SetDarkMode(DarkMode::ALWAYS_LIGHT, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    test->GetDarkMode(mode);
    EXPECT_EQ(mode, DarkMode::ALWAYS_LIGHT);
}

/**
 * @tc.name: ui_appearance_test_002
 * @tc.desc: Test SetDarkMode and GetDarkMode APIs when repeatedly setting dark/light.
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_002, TestSize.Level0)
{
    LOGI("Test SetDarkMode and GetDarkMode APIs when repeatedly setting dark/light.");

    auto test = DarkModeTest::GetUiAppearanceAbilityTest();
    int32_t result = -1;
    test->SetDarkMode(DarkMode::ALWAYS_DARK, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    int32_t mode = -1;
    test->GetDarkMode(mode);
    EXPECT_EQ(mode, DarkMode::ALWAYS_DARK);

    test->SetDarkMode(DarkMode::ALWAYS_DARK, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SYS_ERR);
    test->GetDarkMode(mode);
    EXPECT_EQ(mode, DarkMode::ALWAYS_DARK);

    test->SetDarkMode(DarkMode::ALWAYS_LIGHT, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    test->GetDarkMode(mode);
    EXPECT_EQ(mode, DarkMode::ALWAYS_LIGHT);

    test->SetDarkMode(DarkMode::ALWAYS_LIGHT, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SYS_ERR);
    test->GetDarkMode(mode);
    EXPECT_EQ(mode, DarkMode::ALWAYS_LIGHT);
}

/**
 * @tc.name: ui_appearance_test_003
 * @tc.desc: Test the SetDarkMode API when setting an unexpected value
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_003, TestSize.Level0)
{
    LOGI("Test the SetDarkMode API when setting an unexpected value.");

    int32_t result = -1;
    DarkModeTest::GetUiAppearanceAbilityTest()->SetDarkMode(DarkMode::UNKNOWN, result);
    EXPECT_NE(result, 0);
}

/**
 * @tc.name: ui_appearance_test_004
 * @tc.desc: Test the font API
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_004, TestSize.Level0)
{
    LOGI("Test the font API");

    std::string fontScale;
    int32_t result = -1;
    DarkModeTest::GetUiAppearanceAbilityTest()->GetFontScale(fontScale, result);
    EXPECT_EQ(result, 0);

    std::string fontWeightScale;
    DarkModeTest::GetUiAppearanceAbilityTest()->GetFontWeightScale(fontWeightScale, result);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: ui_appearance_test_005
 * @tc.desc: Test the alarm_timer_manager
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_005, TestSize.Level0)
{
    LOGI("Test the alarm_timer_manager");

    bool result = AlarmTimerManager::IsValidScheduleTime(12, 10);
    EXPECT_EQ(result, false);

    result = AlarmTimerManager::IsValidScheduleTime(DAY_TO_MINUTE + 10, DAY_TO_MINUTE + 12);
    EXPECT_EQ(result, false);

    result = AlarmTimerManager::IsValidScheduleTime(10, DAY_TO_MINUTE + 12);
    EXPECT_EQ(result, false);

    result = AlarmTimerManager::IsValidScheduleTime(10, 12);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: ui_appearance_test_006
 * @tc.desc: Test the alarm_timer_manager
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_006, TestSize.Level0)
{
    LOGI("Test the alarm_timer_manager");

    std::array<uint64_t, TRIGGER_ARRAY_SIZE> triggerTimeInterval = {0, 0};
    std::time_t timestamp = std::time(nullptr);
    if (timestamp == static_cast<std::time_t>(-1)) {
        LOGE("fail to get timestamp");
    }
    std::tm *nowTime = std::localtime(&timestamp);
    auto current = nowTime->tm_hour * 60 + nowTime->tm_min;
    if (nowTime != nullptr) {
        nowTime->tm_hour = 0;
        nowTime->tm_min = 0;
        nowTime->tm_sec = 0;
    }
    std::time_t now_zero = std::mktime(nowTime);
    uint64_t zeroTimestamp = static_cast<uint64_t>(now_zero * SECOND_TO_MILLI);

    uint64_t step = DAY_TO_SECOND * SECOND_TO_MILLI;

    uint64_t startTimestamp = zeroTimestamp + (current + 1) * MINUTE_TO_SECOND * SECOND_TO_MILLI;
    AlarmTimerManager::SetTimerTriggerTime(current + 1, current + 2, triggerTimeInterval);
    EXPECT_EQ(triggerTimeInterval[0], startTimestamp);

    startTimestamp = zeroTimestamp + (current - 1) * MINUTE_TO_SECOND * SECOND_TO_MILLI;
    AlarmTimerManager::SetTimerTriggerTime(current - 1, current + 2, triggerTimeInterval);
    EXPECT_EQ(triggerTimeInterval[0], startTimestamp + step);
}

/**
 * @tc.name: ui_appearance_test_007
 * @tc.desc: Test the alarm_timer_manager
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_007, TestSize.Level0)
{
    LOGI("Test the alarm_timer_manager");

    auto uiAppearanceTimerManager = DarkModeTest::GetAlarmTimerManager();
    int res = uiAppearanceTimerManager->SetScheduleTime(
            10, 12, 100, [](){}, [](){});
    EXPECT_EQ(res, 0);

    res = uiAppearanceTimerManager->RestartAllTimer();
    EXPECT_EQ(res, 1);

    uiAppearanceTimerManager->ClearTimerByUserId(100);
}

/**
 * @tc.name: ui_appearance_test_008
 * @tc.desc: Test the alarm_timer_manager
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_008, TestSize.Level0)
{
    LOGI("Test OnRemoveSystemAbility.");

    auto test = DarkModeTest::GetUiAppearanceAbilityTest();
    test->userSwitchUpdateConfigurationOnceFlag_.insert(100);
    test->OnRemoveSystemAbility(APP_MGR_SERVICE_ID, "");
    EXPECT_EQ(0, test->userSwitchUpdateConfigurationOnceFlag_.size());
}

/**
 * @tc.name: ui_appearance_test_009
 * @tc.desc: Test the alarm_timer_manager
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_009, TestSize.Level0)
{
    LOGI("Test BackGroundAppColorSwitch.");

    auto test = DarkModeTest::GetUiAppearanceAbilityTest();
    bool ret = test->BackGroundAppColorSwitch(nullptr, 0);
    EXPECT_EQ(false, ret);
}

/**
 * @tc.name: ui_appearance_test_010
 * @tc.desc: Test SetFontScale and GetFontScale APIs when repeatedly setting font scale.
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_010, TestSize.Level0)
{
    LOGI("Test SetFontScale and GetFontScale APIs when repeatedly setting font scale.");

    auto test = DarkModeTest::GetUiAppearanceAbilityTest();
    int32_t result = -1;
    test->SetFontScale("", result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SYS_ERR);
    std::string scale = "1";
    test->SetFontScale(scale, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    std::string scaleGet;
    test->GetFontScale(scaleGet, result);
    ASSERT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    EXPECT_EQ(scale, scaleGet);
}

/**
 * @tc.name: ui_appearance_test_011
 * @tc.desc: Test SetFontWeightScale and GetFontWeightScale APIs when repeatedly setting font weight scale.
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_011, TestSize.Level0)
{
    LOGI("Test SetFontWeightScale and GetFontWeightScale APIs when repeatedly setting font weight scale.");

    auto test = DarkModeTest::GetUiAppearanceAbilityTest();
    int32_t result = -1;
    test->SetFontWeightScale("", result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SYS_ERR);
    std::string scale = "1";
    test->SetFontWeightScale(scale, result);
    EXPECT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    std::string scaleGet;
    test->GetFontWeightScale(scaleGet, result);
    ASSERT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    EXPECT_EQ(scale, scaleGet);
}

/**
 * @tc.name: ui_appearance_test_012
 * @tc.desc: Test font weight default value is read from standard_font_weight.
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_012, TestSize.Level0)
{
    LOGI("Test font weight default value is read from standard_font_weight.");

    MockSetParameterValue(STANDARD_FONT_WEIGHT, "1.75");
    UiAppearanceAbility::UiAppearanceParam param;
    EXPECT_EQ(param.fontWeightScale, "1.75");
}

/**
 * @tc.name: ui_appearance_test_013
 * @tc.desc: Test font weight default value falls back to BASE_SCALE when parameter read fails.
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_013, TestSize.Level0)
{
    LOGI("Test font weight default value falls back to BASE_SCALE when parameter read fails.");

    MockSetGetParameterShouldFail(true);
    UiAppearanceAbility::UiAppearanceParam param;
    EXPECT_EQ(param.fontWeightScale, "1");
}

/**
 * @tc.name: ui_appearance_test_014
 * @tc.desc: Test GetFontWeightScale keeps current external fallback behavior.
 * @tc.type: FUNC
 */
HWTEST_F(DarkModeTest, ui_appearance_test_014, TestSize.Level0)
{
    LOGI("Test GetFontWeightScale keeps current external fallback behavior.");

    MockSetParameterValue(STANDARD_FONT_WEIGHT, "1.75");
    auto test = DarkModeTest::GetUiAppearanceAbilityTest();
    std::string scaleGet;
    int32_t result = -1;
    test->GetFontWeightScale(scaleGet, result);
    ASSERT_EQ(result, UiAppearanceAbilityErrCode::SUCCEEDED);
    EXPECT_EQ(scaleGet, "1");
}
} // namespace ArkUi::UiAppearance
} // namespace OHOS
