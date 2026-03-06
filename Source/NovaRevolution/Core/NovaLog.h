// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Nova 프로젝트 전용 로그 및 디버그 매크로
 */

#define NOVA_LOG_CALLINFO (FString(__FUNCTION__) + TEXT(" (") + FString::FromInt(__LINE__) + TEXT(")"))

// 기본 로그 매크로
#define NOVA_LOG(Verbosity, Format, ...) UE_LOG(LogNovaRevolution, Verbosity, TEXT("%s %s"), *NOVA_LOG_CALLINFO, *FString::Printf(TEXT(Format), ##__VA_ARGS__))

// 화면 및 로그 동시 출력 매크로
#define NOVA_SCREEN(Verbosity, Format, ...) \
{ \
    NOVA_LOG(Verbosity, Format, ##__VA_ARGS__); \
    if (GEngine) \
    { \
        FColor Color = FColor::Cyan; \
        FString VStr = TEXT(#Verbosity); \
        if (VStr == TEXT("Warning")) Color = FColor::Yellow; \
        else if (VStr == TEXT("Error")) Color = FColor::Red; \
        GEngine->AddOnScreenDebugMessage(-1, 5.f, Color, FString::Printf(TEXT(Format), ##__VA_ARGS__)); \
    } \
}

// 조건 체크 및 실패 시 에러 로그 및 화면 출력
#define NOVA_CHECK(Condition, Format, ...) \
{ \
    if (!(Condition)) \
    { \
        NOVA_SCREEN(Error, "CHECK FAILED: %s | " Format, TEXT(#Condition), ##__VA_ARGS__); \
        ensureMsgf(Condition, TEXT("CHECK FAILED: %s | " Format), TEXT(#Condition), ##__VA_ARGS__); \
    } \
}
