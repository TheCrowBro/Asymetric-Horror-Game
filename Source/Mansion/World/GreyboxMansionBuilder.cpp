#include "World/GreyboxMansionBuilder.h"
#include "Core/MansionGameMode.h"
#include "Lighting/IlluminationSubsystem.h"
#include "World/LightFixture.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/PlayerStart.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UInstancedStaticMeshComponent* CreateGeometry(AActor* Owner, USceneComponent* Parent, FName Name, FName Tag, UStaticMesh* Mesh)
	{
		UInstancedStaticMeshComponent* Geometry = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Geometry->SetupAttachment(Parent);
		Geometry->SetMobility(EComponentMobility::Static);
		Geometry->SetStaticMesh(Mesh);
		Geometry->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Geometry->ComponentTags.Add(Tag);
		return Geometry;
	}
}

AGreyboxMansionBuilder::AGreyboxMansionBuilder()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	GroundFloorGeometry = CreateGeometry(this, Root, TEXT("GroundFloorGeometry"), TEXT("Floor1"), CubeMesh.Object);
	UpperFloorGeometry = CreateGeometry(this, Root, TEXT("UpperFloorGeometry"), TEXT("Floor2"), CubeMesh.Object);
	RoofGeometry = CreateGeometry(this, Root, TEXT("RoofGeometry"), TEXT("Roof"), CubeMesh.Object);
}

void AGreyboxMansionBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Rebuild();
}

void AGreyboxMansionBuilder::BeginPlay()
{
	Super::BeginPlay();

	RegisterRooms();

	if (HasAuthority())
	{
		if (bSpawnLights)
		{
			SpawnLights();
		}
		if (bSpawnPlayerStarts)
		{
			SpawnPlayerStarts();
		}
	}
}

void AGreyboxMansionBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UIlluminationSubsystem* Illumination = GetWorld()->GetSubsystem<UIlluminationSubsystem>())
	{
		for (const int32 RoomId : RegisteredRoomIds)
		{
			Illumination->UnregisterRoom(RoomId);
		}
	}
	RegisteredRoomIds.Reset();

	Super::EndPlay(EndPlayReason);
}

// --- Geometry ---

void AGreyboxMansionBuilder::Rebuild()
{
	GroundFloorGeometry->ClearInstances();
	UpperFloorGeometry->ClearInstances();
	RoofGeometry->ClearInstances();

	const float LevelHeight = GetLevelHeight();

	for (int32 Floor = 0; Floor < NumFloors; ++Floor)
	{
		UInstancedStaticMeshComponent* Geometry = Floor == 0 ? GroundFloorGeometry : UpperFloorGeometry;
		const float BaseZ = Floor * LevelHeight;

		// Floor slabs, one per room so the stairwell can leave a hole.
		for (int32 X = 0; X < RoomsX; ++X)
		{
			for (int32 Y = 0; Y < RoomsY; ++Y)
			{
				if (Floor > 0 && IsStairRoom(X, Y))
				{
					continue;
				}
				AddBox(Geometry,
					FVector(X * RoomSize, Y * RoomSize, BaseZ - FloorThickness),
					FVector((X + 1) * RoomSize, (Y + 1) * RoomSize, BaseZ));
			}
		}

		// Walls on lines of constant X.
		for (int32 LineX = 0; LineX <= RoomsX; ++LineX)
		{
			for (int32 Y = 0; Y < RoomsY; ++Y)
			{
				const bool bExterior = LineX == 0 || LineX == RoomsX;
				const bool bDoorway = !bExterior && WallHasDoorway(Floor, FIntPoint(LineX - 1, Y), FIntPoint(LineX, Y));
				AddWall(Geometry, /*bConstantX*/ true, LineX * RoomSize, Y * RoomSize, (Y + 1) * RoomSize, BaseZ, bDoorway);
			}
		}

		// Walls on lines of constant Y.
		for (int32 LineY = 0; LineY <= RoomsY; ++LineY)
		{
			for (int32 X = 0; X < RoomsX; ++X)
			{
				const bool bExterior = LineY == 0 || LineY == RoomsY;
				const bool bDoorway = !bExterior && WallHasDoorway(Floor, FIntPoint(X, LineY - 1), FIntPoint(X, LineY));
				AddWall(Geometry, /*bConstantX*/ false, LineY * RoomSize, X * RoomSize, (X + 1) * RoomSize, BaseZ, bDoorway);
			}
		}
	}

	AddBox(RoofGeometry,
		FVector(0.f, 0.f, NumFloors * LevelHeight - FloorThickness),
		FVector(RoomsX * RoomSize, RoomsY * RoomSize, NumFloors * LevelHeight));

	AddRamp();
}

void AGreyboxMansionBuilder::AddBox(UInstancedStaticMeshComponent* Geometry, const FVector& Min, const FVector& Max) const
{
	const FVector Size = Max - Min;
	if (Size.X <= 1.f || Size.Y <= 1.f || Size.Z <= 1.f)
	{
		return;
	}
	// Engine cube is 100uu, centred on its pivot.
	Geometry->AddInstance(FTransform(FRotator::ZeroRotator, (Min + Max) * 0.5f, Size / 100.f));
}

void AGreyboxMansionBuilder::AddWall(UInstancedStaticMeshComponent* Geometry, bool bConstantX, float LineCoord, float SpanStart, float SpanEnd, float BaseZ, bool bDoorway) const
{
	auto AddPiece = [&](float A0, float A1, float Z0, float Z1)
	{
		const float HalfThickness = WallThickness * 0.5f;
		if (bConstantX)
		{
			AddBox(Geometry, FVector(LineCoord - HalfThickness, A0, Z0), FVector(LineCoord + HalfThickness, A1, Z1));
		}
		else
		{
			AddBox(Geometry, FVector(A0, LineCoord - HalfThickness, Z0), FVector(A1, LineCoord + HalfThickness, Z1));
		}
	};

	const float TopZ = BaseZ + WallHeight;
	if (!bDoorway)
	{
		AddPiece(SpanStart, SpanEnd, BaseZ, TopZ);
		return;
	}

	const float Middle = (SpanStart + SpanEnd) * 0.5f;
	const float HalfDoor = DoorWidth * 0.5f;
	AddPiece(SpanStart, Middle - HalfDoor, BaseZ, TopZ);
	AddPiece(Middle + HalfDoor, SpanEnd, BaseZ, TopZ);
	AddPiece(Middle - HalfDoor, Middle + HalfDoor, BaseZ + DoorHeight, TopZ);
}

void AGreyboxMansionBuilder::AddRamp() const
{
	// A straight ramp across the stair room climbing +X, arriving at floor 2's doorway
	// into the neighbouring room.
	const FIntPoint Stair = GetClampedStairRoom();
	const float Rise = GetLevelHeight();
	const float Run = RoomSize;
	const float Angle = FMath::Atan2(Rise, Run);
	const float Length = FMath::Sqrt(Rise * Rise + Run * Run);

	const FVector TopSurfaceCentre(Stair.X * RoomSize + Run * 0.5f, (Stair.Y + 0.5f) * RoomSize, Rise * 0.5f);
	const FVector SurfaceNormal(-FMath::Sin(Angle), 0.f, FMath::Cos(Angle));
	const FVector Centre = TopSurfaceCentre - SurfaceNormal * FloorThickness * 0.5f;

	GroundFloorGeometry->AddInstance(FTransform(
		FRotator(FMath::RadiansToDegrees(Angle), 0.f, 0.f),
		Centre,
		FVector(Length, RoomSize - WallThickness, FloorThickness) / 100.f));
}

FIntPoint AGreyboxMansionBuilder::GetClampedStairRoom() const
{
	return FIntPoint(FMath::Clamp(StairRoom.X, 0, RoomsX - 2), FMath::Clamp(StairRoom.Y, 0, RoomsY - 1));
}

bool AGreyboxMansionBuilder::IsStairRoom(int32 X, int32 Y) const
{
	return GetClampedStairRoom() == FIntPoint(X, Y);
}

bool AGreyboxMansionBuilder::WallHasDoorway(int32 Floor, FIntPoint RoomA, FIntPoint RoomB) const
{
	const bool bTouchesStairwell = IsStairRoom(RoomA.X, RoomA.Y) || IsStairRoom(RoomB.X, RoomB.Y);
	if (Floor == 0 || !bTouchesStairwell)
	{
		return true;
	}

	// Upstairs, the stairwell is a hole: only the doorway at the top of the ramp exists.
	const FIntPoint Stair = GetClampedStairRoom();
	const FIntPoint Landing = Stair + FIntPoint(1, 0);
	return (RoomA == Stair && RoomB == Landing) || (RoomA == Landing && RoomB == Stair);
}

// --- Population ---

void AGreyboxMansionBuilder::RegisterRooms()
{
	UIlluminationSubsystem* Illumination = GetWorld()->GetSubsystem<UIlluminationSubsystem>();
	if (!Illumination)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const float LevelHeight = GetLevelHeight();

	for (int32 Floor = 0; Floor < NumFloors; ++Floor)
	{
		for (int32 X = 0; X < RoomsX; ++X)
		{
			for (int32 Y = 0; Y < RoomsY; ++Y)
			{
				FMansionRoom Room;
				Room.RoomName = *FString::Printf(TEXT("F%d_Room_%d_%d"), Floor + 1, X, Y);
				Room.Bounds = FBox(
					Origin + FVector(X * RoomSize, Y * RoomSize, Floor * LevelHeight),
					Origin + FVector((X + 1) * RoomSize, (Y + 1) * RoomSize, Floor * LevelHeight + WallHeight));
				Room.AmbientLight = RoomAmbientLight;
				Room.FloorIndex = Floor + 1;
				RegisteredRoomIds.Add(Illumination->RegisterRoom(Room));
			}
		}
	}
}

void AGreyboxMansionBuilder::SpawnLights()
{
	FRandomStream Stream(Seed);
	const FVector Origin = GetActorLocation();
	const float LevelHeight = GetLevelHeight();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Floor = 0; Floor < NumFloors; ++Floor)
	{
		for (int32 X = 0; X < RoomsX; ++X)
		{
			for (int32 Y = 0; Y < RoomsY; ++Y)
			{
				if (Stream.FRand() >= LitRoomChance)
				{
					continue;
				}
				const FVector Location = Origin + FVector((X + 0.5f) * RoomSize, (Y + 0.5f) * RoomSize, Floor * LevelHeight + WallHeight - 40.f);
				GetWorld()->SpawnActor<ALightFixture>(Location, FRotator::ZeroRotator, SpawnParams);
			}
		}
	}
}

void AGreyboxMansionBuilder::SpawnPlayerStarts()
{
	FRandomStream Stream(Seed + 1);
	const FVector Origin = GetActorLocation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The House's camera pivots over the middle of the ground floor.
	const FVector HouseLocation = Origin + FVector(RoomsX * RoomSize * 0.5f, RoomsY * RoomSize * 0.5f, 100.f);
	if (APlayerStart* HouseStart = GetWorld()->SpawnActor<APlayerStart>(HouseLocation, FRotator::ZeroRotator, SpawnParams))
	{
		HouseStart->PlayerStartTag = AMansionGameMode::HouseStartTag;
	}

	// Survivors start spread across ground-floor rooms (not the stairwell).
	TArray<FIntPoint> Rooms;
	for (int32 X = 0; X < RoomsX; ++X)
	{
		for (int32 Y = 0; Y < RoomsY; ++Y)
		{
			if (!IsStairRoom(X, Y))
			{
				Rooms.Add(FIntPoint(X, Y));
			}
		}
	}
	for (int32 Index = Rooms.Num() - 1; Index > 0; --Index)
	{
		Rooms.Swap(Index, Stream.RandRange(0, Index));
	}

	for (int32 Index = 0; Index < NumSurvivorStarts && Rooms.Num() > 0; ++Index)
	{
		const FIntPoint Room = Rooms[Index % Rooms.Num()];
		const float Offset = (Index / Rooms.Num()) * 120.f;
		const FVector Location = Origin + FVector((Room.X + 0.5f) * RoomSize + Offset, (Room.Y + 0.5f) * RoomSize, 100.f);
		GetWorld()->SpawnActor<APlayerStart>(Location, FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), SpawnParams);
	}
}
