#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SurvivorCharacter.generated.h"

class UCameraComponent;
class UExposureComponent;
class UFearComponent;
class UGameplayLightComponent;
class UInputAction;
class UInputMappingContext;
class USpotLightComponent;
class UStaticMeshComponent;
struct FInputActionValue;

/** First-person Survivor. Greybox body is a cylinder; carries a flashlight. */
UCLASS()
class MANSION_API ASurvivorCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASurvivorCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UFUNCTION(BlueprintPure, Category = "Survivor")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "Survivor")
	bool IsFlashlightOn() const { return bFlashlightOn; }

	UFUNCTION(BlueprintCallable, Category = "Survivor")
	void SetFlashlightOn(bool bOn);

	UExposureComponent* GetExposure() const { return Exposure; }
	UFearComponent* GetFear() const { return Fear; }

	/** The House must not see Survivors' light glow; called on the House's machine. */
	void HideVisualLightsForHouseView();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GreyboxBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpotLightComponent> FlashlightLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGameplayLightComponent> FlashlightGameplayLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UExposureComponent> Exposure;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFearComponent> Fear;

private:
	void BuildInput();
	bool CanAct() const;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();
	void ToggleCrouch();
	void ToggleFlashlight();

	void SetSprinting(bool bSprinting);
	void ApplyMovementSpeed();
	void ApplyFlashlightState();

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bSprinting);

	UFUNCTION(Server, Reliable)
	void ServerSetFlashlightOn(bool bOn);

	UFUNCTION()
	void OnRep_IsSprinting();

	UFUNCTION()
	void OnRep_FlashlightOn();

	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting)
	bool bIsSprinting = false;

	UPROPERTY(ReplicatedUsing = OnRep_FlashlightOn)
	bool bFlashlightOn = false;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FlashlightAction;
};
