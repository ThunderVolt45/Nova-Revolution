// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/NovaAttributeSet.h"

UNovaAttributeSet::UNovaAttributeSet()
{
	// 기본값 초기화
	InitHealth(100.0f);
	InitWatt(100.0f);
	InitAttack(10.0f);
	InitDefense(0.0f);
	InitSpeed(300.0f);
	InitFireRate(1.0f);
	InitSight(2400.0f);
	InitRange(1800.0f);
	InitMinRange(0.0f);
	InitSplashRange(0.0f);
}
