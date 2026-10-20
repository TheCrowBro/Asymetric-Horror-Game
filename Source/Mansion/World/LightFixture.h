#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LightFixture.generated.h"

class UGameplayLightComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/** Ceiling light. On/off is server authoritative and replicated (House abilities will flick it). */
UCLASS()
class MANSION_API ALightFixture : public AActor
{
	GENERATED_BODY()

public:
	ALightFixture();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Light")
	void SetLightOn(bool bOn);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Light")
	void ToggleLight() { SetLightOn(!bIsOn); }

	UFUNCTION(BlueprintPure, Category = "Light")
	bool IsLightOn() const { return bIsOn; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bulb;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGameplayLightComponent> GameplayLight;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_IsOn, Category = "Light")
	bool bIsOn = true;

private:
	UFUNCTION()
	void OnRep_IsOn();
};
