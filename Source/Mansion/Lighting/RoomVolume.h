#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomVolume.generated.h"

class UBoxComponent;

/**
 * Hand-placed room for maps that don't use AGreyboxMansionBuilder (which registers
 * its own rooms). Size the box to cover the room from floor to ceiling.
 */
UCLASS()
class MANSION_API ARoomVolume : public AActor
{
	GENERATED_BODY()

public:
	ARoomVolume();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Room")
	TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "0"))
	float AmbientLight = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "0"))
	float AmbientNoiseFloor = 0.f;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "1"))
	int32 FloorIndex = 1;

private:
	int32 RoomId = INDEX_NONE;
};
