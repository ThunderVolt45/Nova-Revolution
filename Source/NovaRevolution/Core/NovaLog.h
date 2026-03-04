// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Nova 프로젝트 전용 로그 및 디버그 매크로
 */

#define NOVA_LOG_CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))

// 출력창 로그 (Verbosity: Log, Warning, Error 등)
#define NOVA_LOG(Verbosity, Format, ...) UE_LOG(LogNovaRevolution, Verbosity, TEXT("%s %s"), *NOVA_LOG_CALLINFO, *FString::Printf(TEXT(Format), ##__VA_ARGS__))

// 화면 및 출력창 로그 (Verbosity에 따라 색상 변경)
// Log: Cyan, Warning: Yellow, Error: Red
#define NOVA_SCREEN(Verbosity, Format, ...) \
{ \
    const FString Msg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
    FColor ScreenColor = FColor::Cyan; \
    if (ELogVerbosity::Warning == ELogVerbosity::Verbosity) ScreenColor = FColor::Yellow; \
    else if (ELogVerbosity::Error == ELogVerbosity::Verbosity) ScreenColor = FColor::Red; \
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, ScreenColor, FString::Printf(TEXT("%s %s"), *NOVA_LOG_CALLINFO, *Msg)); \
    UE_LOG(LogNovaRevolution, Verbosity, TEXT("%s %s"), *NOVA_LOG_CALLINFO, *Msg); \
}

// 간단한 불리언 체크 및 로그 (ensure와 유사하나 커스텀 로그 남김)
#define NOVA_CHECK(Condition, Format, ...) \
{ \
    if (!(Condition)) \
    { \
        NOVA_LOG(Error, TEXT("CHECK FAILED: %s | ") TEXT(Format), TEXT(#Condition), ##__VA_ARGS__); \
    } \
}
