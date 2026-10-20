#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GreyboxMansionBuilder.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Drop one into an empty level (at the origin, unrotated) for a playable two-floor greybox
 * mansion: a grid of rooms with doorways, a ramp up to floor 2, and a roof.
 *
 * At BeginPlay it also registers every room with the illumination subsystem and (on the
 * server) spawns ceiling lights in a random subset of rooms plus the player starts.
 *
 * Geometry components are tagged Floor1 / Floor2 / Roof so the House's cutaway can hide them.
 */
UCLASS()
class MANSION_API AGreyboxMansionBuilder : public AActor
{
	GENERATED_BODY()

public:
	AGreyboxMansionBuilder();

	virtual void OnConstruction(const FTransform& Transform) override;

	float GetLevelHeight() const { return WallHeight + FloorThickness; }
	int32 GetNumFloors() const { return NumFloors; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GroundFloorGeometry;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> UpperFloorGeometry;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> RoofGeometry;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "2", ClampMax = "10"))
	int32 RoomsX = 4;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "1", ClampMax = "10"))
	int32 RoomsY = 3;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "300", Units = "cm"))
	float RoomSize = 800.f;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "250", Units = "cm"))
	float WallHeight = 350.f;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "5", Units = "cm"))
	float WallThickness = 20.f;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "5", Units = "cm"))
	float FloorThickness = 20.f;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "80", Units = "cm"))
	float DoorWidth = 140.f;

	UPROPERTY(EditAnywhere, Category = "Layout", meta = (ClampMin = "200", Units = "cm"))
	float DoorHeight = 230.f;

	/** Room holding the ramp; the ramp climbs towards +X, so X must be < RoomsX - 1. */
	UPROPERTY(EditAnywhere, Category = "Layout")
	FIntPoint StairRoom = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, Category = "Population")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, Category = "Population")
	bool bSpawnLights = true;

	UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "0", ClampMax = "1"))
	float LitRoomChance = 0.6f;

	/** Room baseline light with every fixture off. Below the dark threshold = dark room. */
	UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "0"))
	float RoomAmbientLight = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Population")
	bool bSpawnPlayerStarts = true;

	UPROPERTY(EditAnywhere, Category = "Population", meta = (ClampMin = "1"))
	int32 NumSurvivorStarts = 5;

private:
	static constexpr int32 NumFloors = 2;

	void Rebuild();
	void AddBox(UInstancedStaticMeshComponent* Geometry, const FVector& Min, const FVector& Max) const;
	void AddWall(UInstancedStaticMeshComponent* Geometry, bool bConstantX, float LineCoord, float SpanStart, float SpanEnd, float BaseZ, bool bDoorway) const;
	void AddRamp() const;
	bool IsStairRoom(int32 X, int32 Y) const;
	bool WallHasDoorway(int32 Floor, FIntPoint RoomA, FIntPoint RoomB) const;
	FIntPoint GetClampedStairRoom() const;

	void RegisterRooms();
	void SpawnLights();
	void SpawnPlayerStarts();

	TArray<int32> RegisteredRoomIds;
};
