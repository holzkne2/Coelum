// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CBaseEnemy_Controller.generated.h"

/**
 * 
 */
UCLASS()
class FANTASY_API ACBaseEnemy_Controller : public AAIController
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	static bool SetSightAngle(AAIController* Controller, float Angle);
};
