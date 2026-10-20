#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RevealMarker.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Client-local beacon the House sees where a revealed Survivor stands. When the reveal
 * ends it becomes a "last known position" ghost that shrinks away. Never replicated.
 */
UCLASS(NotPlaceable)
class MANSION_API ARevealMarker : public AActor
{
	GENERATED_BODY()

public:
	ARevealMarker();

	virtual void Tick(float DeltaSeconds) override;

	void SetLive(const FVector& Location, const FLinearColor& Color);
	void StartGhost(float Duration);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Marker")
	TObjectPtr<UStaticMeshComponent> Beacon;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

private:
	FLinearColor CurrentColor = FLinearColor::White;
	bool bGhost = false;
	float GhostDuration = 1.f;
	float GhostElapsed = 0.f;
};
