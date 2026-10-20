#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IlluminationSubsystem.generated.h"

class UGameplayLightComponent;

/** A room as far as gameplay is concerned: bounds plus ambient light/noise. */
USTRUCT(BlueprintType)
struct MANSION_API FMansionRoom
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FName RoomName;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	FBox Bounds = FBox(ForceInit);

	/** Baseline illumination with every light off. Below the dark threshold = dark room. */
	UPROPERTY(BlueprintReadOnly, Category = "Room")
	float AmbientLight = 0.f;

	/** Raised by Shatter Glass; masks monster audio cues (milestone 6). */
	UPROPERTY(BlueprintReadOnly, Category = "Room")
	float AmbientNoiseFloor = 0.f;

	/** 1-based floor number, used by the House's cutaway view. */
	UPROPERTY(BlueprintReadOnly, Category = "Room")
	int32 FloorIndex = 1;
};

/**
 * Gameplay-side lighting. Lumen is for visuals only; everything that cares about
 * "how lit is this spot" (darkness reveal, Fear) asks this subsystem instead.
 *
 * Illumination = room ambient + sum over enabled lights of
 *   Intensity * (1 - d/Radius)^2 * cone factor, if the light has line of sight.
 */
UCLASS()
class MANSION_API UIlluminationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UIlluminationSubsystem, STATGROUP_Tickables); }

	void RegisterLight(UGameplayLightComponent* Light);
	void UnregisterLight(UGameplayLightComponent* Light);

	/** Returns a handle for UnregisterRoom. */
	int32 RegisterRoom(const FMansionRoom& Room);
	void UnregisterRoom(int32 RoomId);

	const FMansionRoom* FindRoomAt(const FVector& Location) const;

	/**
	 * QueryActor is ignored by occlusion traces, and lights it owns (held flashlight/lantern)
	 * contribute Intensity * SelfIlluminationFactor instead of the distance falloff.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mansion|Lighting")
	float GetIlluminationAt(const FVector& Location, const AActor* QueryActor) const;

	static float ComputeFalloff(float Intensity, float Radius, float Distance);
	static float ComputeConeFactor(const FVector& LightForward, const FVector& DirectionToTarget, float InnerConeDegrees, float OuterConeDegrees);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	TArray<TWeakObjectPtr<UGameplayLightComponent>> Lights;
	TMap<int32, FMansionRoom> Rooms;
	int32 NextRoomId = 1;
};
