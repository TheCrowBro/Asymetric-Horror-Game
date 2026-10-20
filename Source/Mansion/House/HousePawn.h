#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HousePawn.generated.h"

class ARevealMarker;
class ASurvivorCharacter;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/**
 * The House's board-game view: a steep top-down camera that pans, zooms and steps
 * between floors (everything above the current floor is cut away).
 *
 * On the House's machine, Survivors are hidden; only revealed Survivors show up as
 * coloured beacons, which fade into ghosts when the reveal ends.
 */
UCLASS()
class MANSION_API AHousePawn : public APawn
{
	GENERATED_BODY()

public:
	AHousePawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UFUNCTION(BlueprintPure, Category = "House")
	int32 GetCurrentFloor() const { return CurrentFloor; }

	UFUNCTION(BlueprintPure, Category = "House")
	int32 GetNumFloors() const { return NumFloors; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(EditAnywhere, Category = "House|Camera")
	float MinZoom = 800.f;

	UPROPERTY(EditAnywhere, Category = "House|Camera")
	float MaxZoom = 6000.f;

	UPROPERTY(EditAnywhere, Category = "House|Camera")
	float ZoomStep = 300.f;

	/** Pan speed as a fraction of the current zoom distance, per second. */
	UPROPERTY(EditAnywhere, Category = "House|Camera")
	float PanSpeedFactor = 1.2f;

	/** Used when there's no AGreyboxMansionBuilder in the level to ask. */
	UPROPERTY(EditAnywhere, Category = "House|Floors")
	float FallbackFloorHeight = 370.f;

	UPROPERTY(EditAnywhere, Category = "House|Floors")
	int32 FallbackNumFloors = 2;

private:
	void BuildInput();
	void Pan(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);
	void FloorUp();
	void FloorDown();

	void SetCurrentFloor(int32 NewFloor);
	float GetFloorBaseZ(int32 Floor) const;
	void ApplyFloorCutaway();
	void UpdateSurvivorMarkers();

	int32 CurrentFloor = 1;
	int32 NumFloors = 2;
	float FloorHeight = 370.f;
	float GroundZ = 0.f;

	FTimerHandle CutawayRefreshTimer;

	TMap<TWeakObjectPtr<ASurvivorCharacter>, TWeakObjectPtr<ARevealMarker>> Markers;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> InputContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PanAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FloorUpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FloorDownAction;
};
