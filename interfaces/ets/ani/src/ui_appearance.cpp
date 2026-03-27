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

#include "ui_appearance.h"
#include "ui_appearance_ability_client.h"
namespace OHOS {
namespace ArkUi::UiAppearance {
namespace {
static constexpr size_t MAX_FONT_SCALE = 5;
static constexpr size_t MIN_FONT_SCALE = 0;
constexpr char PERMISSION_ERR_MSG[] =
    "An attempt was made to update configuration forbidden by permission: ohos.permission.UPDATE_CONFIGURATION.";
constexpr char INVALID_ARG_MSG[] = "The type of 'mode' must be DarkMode.";

std::string ParseErrCode(const int32_t errCode)
{
    switch (errCode) {
        case UiAppearanceAbilityErrCode::PERMISSION_ERR:
            return "Permission denied. ";
        case UiAppearanceAbilityErrCode::INVALID_ARG:
            return "Parameter error. ";
        case UiAppearanceAbilityErrCode::SYS_ERR:
            return "Internal error. ";
        default:
            return "";
    }
}
}

ani_object GetErrorObject(ani_env *env, const std::string &errMsg, int32_t code)
{
    ani_status status = ANI_ERROR;
    if (env == nullptr) {
        LOGE("env NULL %{public}d", status);
        return nullptr;
    }
    ani_class errClass;
    if (ANI_OK != (status = env->FindClass("@ohos.base.BusinessError", &errClass))) {
        LOGE("FindClass failed %{public}d", status);
        return nullptr;
    }
    ani_method ctor;
    if (ANI_OK != (status = env->Class_FindMethod(errClass, "<ctor>", ":", &ctor))) {
        LOGE("Class_FindMethod failed %{public}d", status);
        return nullptr;
    }
    ani_string errMessage;
    if (ANI_OK != (status = env->String_NewUTF8(errMsg.c_str(), errMsg.size(), &errMessage))) {
        LOGE("String_NewUTF8 failed %{public}d", status);
        return nullptr;
    }
    ani_object errObj;
    if (ANI_OK != (status = env->Object_New(errClass, ctor, &errObj))) {
        LOGE("Object_New failed %{public}d", status);
        return nullptr;
    }
    if (ANI_OK != (status = env->Object_SetPropertyByName_Int(errObj, "code", static_cast<ani_int>(code)))) {
        LOGE("SetPropertyByName failed %{public}d", status);
        return nullptr;
    }
    if (ANI_OK != (status = env->Object_SetPropertyByName_Ref(errObj, "message", errMessage))) {
        LOGE("Object_SetPropertyByName_Ref failed %{public}d", status);
        return nullptr;
    }
    return errObj;
}

void AniThrow(ani_env *env, const std::string &errMsg, int32_t code)
{
    if (env == nullptr) {
        return;
    }
    auto errObj = static_cast<ani_error>(GetErrorObject(env, errMsg, code));
    if (errObj == nullptr) {
        LOGE("get error object failed!");
        return;
    }
    if (ANI_OK != env->ThrowError(errObj)) {
        LOGE("throw ani error object failed!");
        return;
    }
}

void OnComplete([[maybe_unused]] ani_env* env, AsyncContext* asyncContext)
{
    if (env == nullptr) {
        delete asyncContext;
        return;
    }
    ani_boolean errorExists;
    env->ExistUnhandledError(&errorExists);
    ani_status OnStatus = ANI_OK;

    if (asyncContext->status == UiAppearanceAbilityErrCode::SUCCEEDED) {
        std::vector<ani_ref> resultRef(1);
        env->GetNull(&resultRef[0]);
        if (asyncContext->deferred) { // promise
            if ((OnStatus = env->PromiseResolver_Resolve(asyncContext->deferred, resultRef[0])) != ANI_OK) {
                LOGE("PromiseResolver_Resolve failed %{public}d", OnStatus);
            }
        } else if (asyncContext->callbackRef) { // AsyncCallback
            ani_ref fnReturnVal;
            if ((OnStatus = env->FunctionalObject_Call(
                static_cast<ani_fn_object>(asyncContext->callbackRef),
                resultRef.size(), resultRef.data(), &fnReturnVal)) != ANI_OK) {
                LOGE("FunctionalObject_Call failed %{public}d", OnStatus);
            }
        }
    } else {
        ani_string msg {};
        std::string strMsg = ParseErrCode(asyncContext->status) + asyncContext->errMsg;
        env->String_NewUTF8(strMsg.c_str(), strMsg.length(), &msg);
        std::vector<ani_ref> errorRef(1);
        env->GetUndefined(&errorRef[0]);
        errorRef[0] = GetErrorObject(env, strMsg, asyncContext->status);
        ani_error error = static_cast<ani_error>(errorRef[0]);
        if (asyncContext->deferred) { // promise
            if ((OnStatus = env->PromiseResolver_Reject(asyncContext->deferred, error)) != ANI_OK) {
                LOGE("PromiseResolver_Reject failed %{public}d", OnStatus);
            }
        } else if (asyncContext->callbackRef) { // AsyncCallback
            ani_ref fnReturn_Val;
            if ((OnStatus = env->FunctionalObject_Call(
                static_cast<ani_fn_object>(asyncContext->callbackRef),
                errorRef.size(), errorRef.data(), &fnReturn_Val)) != ANI_OK) {
                LOGE("FunctionalObject_Call failed %{public}d", OnStatus);
            }
        }
    }
    delete asyncContext;
}

DarkMode ConvertJsDarkMode2Enum(int32_t jsVal)
{
    switch (jsVal) {
        case 0:
            return DarkMode::ALWAYS_DARK;
        case 1:
            return DarkMode::ALWAYS_LIGHT;
        default:
            return DarkMode::UNKNOWN;
    }
}

void SetDarkMode([[maybe_unused]] ani_env* env, ani_enum_item mode, ani_object callbackObj)
{
    if (!env) {
        return;
    }
    auto asyncContext = new (std::nothrow) AsyncContext();
    if (asyncContext == nullptr) {
        AniThrow(env, "create AsyncContext failed.", UiAppearanceAbilityErrCode::SYS_ERR);
        return;
    }
    ani_int modeEnum;
    env->EnumItem_GetValue_Int(mode, &modeEnum);
    asyncContext->ani_SetArg = modeEnum;
    asyncContext->mode = ConvertJsDarkMode2Enum(asyncContext->ani_SetArg);

    if (callbackObj) {
        ani_ref objectGRef;
        env->GlobalReference_Create(reinterpret_cast<ani_ref>(callbackObj), &objectGRef);
        asyncContext->callbackRef = reinterpret_cast<ani_object>(objectGRef);
    }

    auto resCode = UiAppearanceAbilityClient::GetInstance()->SetDarkMode(asyncContext->mode);
    asyncContext->status = static_cast<UiAppearanceAbilityErrCode>(resCode);

    if (asyncContext->status == UiAppearanceAbilityErrCode::PERMISSION_ERR) {
        asyncContext->errMsg = PERMISSION_ERR_MSG;
    } else if (asyncContext->status == UiAppearanceAbilityErrCode::INVALID_ARG) {
        asyncContext->errMsg = INVALID_ARG_MSG;
    } else {
        asyncContext->errMsg = "";
    }
    OnComplete(env, asyncContext);
    return;
}

ani_object SetDarkModeWithPromise([[maybe_unused]] ani_env* env, ani_enum_item mode)
{
    ani_ref resultref = nullptr;
    if (!env) {
        return nullptr;
    }
    env->GetUndefined(&resultref);
    ani_object result = static_cast<ani_object>(resultref);
    auto asyncContext = new (std::nothrow) AsyncContext();
    if (asyncContext == nullptr) {
        AniThrow(env, "create AsyncContext failed.", UiAppearanceAbilityErrCode::SYS_ERR);
        return result;
    }

    ani_int modeEnum;
    env->EnumItem_GetValue_Int(mode, &modeEnum);
    asyncContext->ani_SetArg = modeEnum;
    asyncContext->mode = ConvertJsDarkMode2Enum(asyncContext->ani_SetArg);

    if (asyncContext->callbackRef == nullptr) {
        if (ANI_OK != env->Promise_New(&asyncContext->deferred, &result)) {
            LOGE("Promise_New failed");
        }
    }

    ani_string resource {};
    env->String_NewUTF8("SetDarkModeWithPromise", 22U, &resource);

    auto resCode = UiAppearanceAbilityClient::GetInstance()->SetDarkMode(asyncContext->mode);
    asyncContext->status = static_cast<UiAppearanceAbilityErrCode>(resCode);

    if (asyncContext->status == UiAppearanceAbilityErrCode::PERMISSION_ERR) {
        asyncContext->errMsg = PERMISSION_ERR_MSG;
    } else if (asyncContext->status == UiAppearanceAbilityErrCode::INVALID_ARG) {
        asyncContext->errMsg = INVALID_ARG_MSG;
    } else {
        asyncContext->errMsg = "";
    }
    OnComplete(env, asyncContext);
    return result;
}

ani_enum_item GetDarkMode([[maybe_unused]] ani_env* env)
{
    ani_ref resultref = nullptr;
    ani_enum_item result {};
    if (!env) {
        return result;
    }
    env->GetUndefined(&resultref);
    auto mode = UiAppearanceAbilityClient::GetInstance()->GetDarkMode();
    if (mode == UiAppearanceAbilityErrCode::SYS_ERR) {
        AniThrow(env, "get dark-mode failed.", UiAppearanceAbilityErrCode::SYS_ERR);
        return result;
    }
    if (mode == UiAppearanceAbilityErrCode::PERMISSION_ERR) {
        AniThrow(env,
            "An attempt was made to get configuration forbidden by permission: ohos.permission.UPDATE_CONFIGURATION.",
            UiAppearanceAbilityErrCode::PERMISSION_ERR);
        return result;
    }
    ani_status status = ANI_OK;
    static const char* enumName = "@ohos.uiAppearance.uiAppearance.DarkMode";
    ani_enum enumType;
    if ((status = env->FindEnum(enumName, &enumType)) != ANI_OK) {
        LOGE("find DarkMode enum fail. status = %{public}d", status);
        return result;
    }
    if ((status = env->Enum_GetEnumItemByIndex(
        enumType, ani_size(static_cast<DarkMode>(mode)), &result)) != ANI_OK) {
        LOGE("get DarkMode enum item fail. status = %{public}d", status);
        return result;
    }
    return result;
}

ani_object SetFontScale([[maybe_unused]] ani_env* env, ani_double fontScale)
{
    if (!env) {
        return nullptr;
    }
    ani_ref resultref = nullptr;
    env->GetUndefined(&resultref);
    ani_object result = static_cast<ani_object>(resultref);
    auto asyncContext = new (std::nothrow) AsyncContext();
    if (asyncContext == nullptr) {
        AniThrow(env, "create AsyncContext failed.", UiAppearanceAbilityErrCode::SYS_ERR);
        return result;
    }
    asyncContext->ani_FontScale = fontScale;
    asyncContext->fontScale = std::to_string(asyncContext->ani_FontScale);
    if (asyncContext->callbackRef == nullptr) {
        if (ANI_OK != env->Promise_New(&asyncContext->deferred, &result)) {
            LOGE("Promise_New failed");
            return nullptr;
        }
    }
    int32_t resCode = 0;
    if (asyncContext->ani_FontScale <= MIN_FONT_SCALE || asyncContext->ani_FontScale > MAX_FONT_SCALE) {
        resCode = UiAppearanceAbilityErrCode::INVALID_ARG;
    } else {
        resCode = UiAppearanceAbilityClient::GetInstance()->SetFontScale(asyncContext->fontScale);
    }
    asyncContext->status = static_cast<UiAppearanceAbilityErrCode>(resCode);
    if (asyncContext->status == UiAppearanceAbilityErrCode::PERMISSION_ERR) {
        asyncContext->errMsg = PERMISSION_ERR_MSG;
    } else if (asyncContext->status == UiAppearanceAbilityErrCode::INVALID_ARG) {
        asyncContext->errMsg = "fontScale must between 0 and 5";
    } else {
        asyncContext->errMsg = "";
    }
    if (env != nullptr) {
        OnComplete(env, asyncContext);
    }
    return result;
}

ani_double GetFontScale([[maybe_unused]] ani_env* env)
{
    ani_ref resultref = nullptr;
    ani_double result = -1;
    if (!env) {
        return result;
    }
    env->GetUndefined(&resultref);
    std::string fontScale;
    auto ret = UiAppearanceAbilityClient::GetInstance()->GetFontScale(fontScale);
    if (ret == UiAppearanceAbilityErrCode::SYS_ERR) {
        AniThrow(env, "get font-scale failed.", UiAppearanceAbilityErrCode::SYS_ERR);
        return result;
    }
    if (ret == UiAppearanceAbilityErrCode::PERMISSION_ERR) {
        AniThrow(env,
            "An attempt was made to get configuration forbidden by permission: ohos.permission.UPDATE_CONFIGURATION.",
            UiAppearanceAbilityErrCode::PERMISSION_ERR);
        return result;
    }
    double fontScaleNumber = std::stod(fontScale);
    result = ani_double(fontScaleNumber);
    return result;
}

ani_object SetFontWeightScale([[maybe_unused]] ani_env* env, ani_double fontWeightScale)
{
    if (!env) {
        return nullptr;
    }
    ani_ref resultref = nullptr;
    env->GetUndefined(&resultref);
    ani_object result = static_cast<ani_object>(resultref);
    if (env == nullptr) {
        return result;
    }
    auto asyncContext = new (std::nothrow) AsyncContext();
    if (asyncContext == nullptr) {
        AniThrow(env, "create AsyncContext failed.", UiAppearanceAbilityErrCode::SYS_ERR);
        return result;
    }
    asyncContext->ani_FontWeightScale = fontWeightScale;
    asyncContext->fontWeightScale = std::to_string(asyncContext->ani_FontWeightScale);
    if (asyncContext->callbackRef == nullptr) {
        if (ANI_OK != env->Promise_New(&asyncContext->deferred, &result)) {
            LOGE("Promise_New failed");
            return nullptr;
        }
    }

    int32_t resCode = 0;
    if (asyncContext->ani_FontWeightScale <= MIN_FONT_SCALE ||
        asyncContext->ani_FontWeightScale > MAX_FONT_SCALE) {
        resCode = UiAppearanceAbilityErrCode::INVALID_ARG;
    } else {
        resCode = UiAppearanceAbilityClient::GetInstance()
            ->SetFontWeightScale(asyncContext->fontWeightScale);
    }
    asyncContext->status = static_cast<UiAppearanceAbilityErrCode>(resCode);
    if (asyncContext->status == UiAppearanceAbilityErrCode::PERMISSION_ERR) {
        asyncContext->errMsg = PERMISSION_ERR_MSG;
    } else if (asyncContext->status == UiAppearanceAbilityErrCode::INVALID_ARG) {
        asyncContext->errMsg = "fontWeightScale must between 0 and 5";
    } else {
        asyncContext->errMsg = "";
    }
    OnComplete(env, asyncContext);
    return result;
}

ani_double GetFontWeightScale([[maybe_unused]] ani_env* env)
{
    ani_ref resultref = nullptr;
    ani_double result = -1;
    if (env == nullptr) {
        return result;
    }
    env->GetUndefined(&resultref);
    std::string fontWeightScale;
    auto ret = UiAppearanceAbilityClient::GetInstance()->GetFontWeightScale(fontWeightScale);
    if (ret == UiAppearanceAbilityErrCode::SYS_ERR) {
        AniThrow(env, "get font-Weight-scale failed.", UiAppearanceAbilityErrCode::SYS_ERR);
        return result;
    }
    if (ret == UiAppearanceAbilityErrCode::PERMISSION_ERR) {
        AniThrow(env,
            "An attempt was made to get configuration forbidden by permission: ohos.permission.UPDATE_CONFIGURATION.",
            UiAppearanceAbilityErrCode::PERMISSION_ERR);
        return result;
    }
    double fontWeightScaleNumber = std::stod(fontWeightScale);
    result = ani_double(fontWeightScaleNumber);
    return result;
}
} // namespace ArkUi::UiAppearance
} // namespace OHOS

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        return ANI_ERROR;
    }
    arkts::ani_signature::Namespace myNamespace =
        arkts::ani_signature::Builder::BuildNamespace("@ohos.uiAppearance.uiAppearance");
    std::string namespaceDescriptor = myNamespace.Descriptor();
    ani_namespace ns;
    if (ANI_OK != env->FindNamespace(namespaceDescriptor.c_str(), &ns)) {
        return ANI_ERROR;
    }
    arkts::ani_signature::SignatureBuilder setDarkMode_SignatureBuilder {};
    setDarkMode_SignatureBuilder
        .AddEnum("@ohos.uiAppearance.uiAppearance.DarkMode")
        .AddClass("std.core.Function2");
    std::string setDarkMode_SignatureStr = setDarkMode_SignatureBuilder.BuildSignatureDescriptor();

    arkts::ani_signature::SignatureBuilder setDarkModeWithPromise_SignatureBuilder {};
    setDarkModeWithPromise_SignatureBuilder
        .AddEnum("@ohos.uiAppearance.uiAppearance.DarkMode")
        .SetReturnClass("std.core.Promise");
    std::string setDarkModeWithPromise_SignatureStr =
        setDarkModeWithPromise_SignatureBuilder.BuildSignatureDescriptor();

    std::array methods = {
        ani_native_function { "setDarkMode", setDarkMode_SignatureStr.c_str(),
            reinterpret_cast<void*>(OHOS::ArkUi::UiAppearance::SetDarkMode) },
        ani_native_function { "setDarkMode", setDarkModeWithPromise_SignatureStr.c_str(),
            reinterpret_cast<void*>(OHOS::ArkUi::UiAppearance::SetDarkModeWithPromise) },
        ani_native_function { "getDarkMode", nullptr, reinterpret_cast<void*>(OHOS::ArkUi::UiAppearance::GetDarkMode) },
        ani_native_function {
            "setFontScale", nullptr, reinterpret_cast<void*>(OHOS::ArkUi::UiAppearance::SetFontScale) },
        ani_native_function {
            "getFontScale", nullptr, reinterpret_cast<void*>(OHOS::ArkUi::UiAppearance::GetFontScale) },
        ani_native_function {
            "setFontWeightScale", nullptr, reinterpret_cast<void*>(OHOS::ArkUi::UiAppearance::SetFontWeightScale) },
        ani_native_function {
            "getFontWeightScale", nullptr, reinterpret_cast<void*>(OHOS::ArkUi::UiAppearance::GetFontWeightScale) },
    };
    if (ANI_OK != env->Namespace_BindNativeFunctions(ns, methods.data(), methods.size())) {
        return ANI_ERROR;
    }

    *result = ANI_VERSION_1;
    return ANI_OK;
}