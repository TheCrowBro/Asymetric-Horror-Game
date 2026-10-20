#include "Lighting/RoomVolume.h"
#include "Lighting/IlluminationSubsystem.h"
#include "Components/BoxComponent.h"

ARoomVolume::ARoomVolume()
{
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(400.f, 400.f, 175.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true);
	RootComponent = Bounds;
}

void ARoomVolume::BeginPlay()
{
	Super::BeginPlay();

	if (UIlluminationSubsystem* Illumination = GetWorld()->GetSubsystem<UIlluminationSubsystem>())
	{
		FMansionRoom Room;
		Room.RoomName = GetFName();
		Room.Bounds = Bounds->Bounds.GetBox();
		Room.AmbientLight = AmbientLight;
		Room.AmbientNoiseFloor = AmbientNoiseFloor;
		Room.FloorIndex = FloorIndex;
		RoomId = Illumination->RegisterRoom(Room);
	}
}

void ARoomVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UIlluminationSubsystem* Illumination = GetWorld()->GetSubsystem<UIlluminationSubsystem>())
	{
		Illumination->UnregisterRoom(RoomId);
	}

	Super::EndPlay(EndPlayReason);
}
