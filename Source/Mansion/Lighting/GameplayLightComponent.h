#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayLightComponent.generated.h"

class ULightComponent;

/**
 * The gameplay half of a light. Attach next to a visual ULightComponent and link them;
 * SetLightEnabled keeps both in sync. Not replicated itself: the owning actor replicates
 * its on/off state and calls SetLightEnabled on every machine.
 */
UCLASS(ClassGroup = Mansion, meta = (BlueprintSpawnableComponent))
class MANSION_API UGameplayLightComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Gameplay Light")
	void SetLightEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Gameplay Light")
	bool IsLightEnabled() const { return bLightEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Gameplay Light")
	void SetLinkedLight(ULightComponent* InLinkedLight);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (ClampMin = "0"))
	float Intensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light")
	bool bSpot = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (EditCondition = "bSpot", ClampMin = "0", ClampMax = "89", Units = "deg"))
	float InnerConeAngle = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (EditCondition = "bSpot", ClampMin = "0", ClampMax = "89", Units = "deg"))
	float OuterConeAngle = 30.f;

	/** How much this light illuminates the actor carrying it (lantern ~1, flashlight ~0.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Light", meta = (ClampMin = "0", ClampMax = "1"))
	float SelfIlluminationFactor = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Gameplay Light")
	bool bLightEnabled = true;

	UPROPERTY(Transient)
	TObjectPtr<ULightComponent> LinkedLight;
};
