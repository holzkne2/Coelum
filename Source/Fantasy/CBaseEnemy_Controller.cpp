// Fill out your copyright notice in the Description page of Project Settings.


#include "CBaseEnemy_Controller.h"
#include <Runtime/AIModule/Classes/Perception/AIPerceptionComponent.h>
#include <Runtime/AIModule/Classes/Perception/AISense.h>
#include <Runtime/AIModule/Classes/Perception/AISense_Sight.h>
#include <Runtime/AIModule/Classes/Perception/AISenseConfig_Sight.h>

bool ACBaseEnemy_Controller::SetSightAngle(AAIController* Controller, float Angle)
{
    if (Controller == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Controller == nullptr"));
        return false;
    }

    FAISenseID Id = UAISense::GetSenseID(UAISense_Sight::StaticClass());
    if (!Id.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Wrong Sense ID"));
        return false;
    }

    UAIPerceptionComponent* Perception = Controller->GetAIPerceptionComponent();
    if (Perception == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Perception == nullptr"));
        return false;
    }

    auto Config = Perception->GetSenseConfig(Id);
    if (Config == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Config == nullptr"));
        return false;
    }

    auto ConfigSight = Cast<UAISenseConfig_Sight>(Config);
    ConfigSight->PeripheralVisionAngleDegrees = Angle;

    Perception->RequestStimuliListenerUpdate();

    return true;
}