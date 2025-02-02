// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "sensors/SensorFactory.hpp"
#include <memory>
#include "NedTransform.h"
#include "GameFramework/Actor.h"

class UnrealSensorFactory : public msr::airlib::SensorFactory {
public:
    typedef msr::airlib::AirSimSettings AirSimSettings;

public:
    virtual ~UnrealSensorFactory() {};
    UnrealSensorFactory(AActor* actor, const NedTransform* ned_transform);
    UnrealSensorFactory(TMap<FString, AActor*> actors);
    void setActors(TMap<FString, AActor*> actor, const NedTransform* ned_transform);
    virtual std::unique_ptr<msr::airlib::SensorBase> createSensorFromSettings(
        const AirSimSettings::SensorSetting* sensor_setting) const override;

private:
    //AActor* actor_;
    TMap<FString, AActor*> actors_;
    const NedTransform* ned_transform_;
};
