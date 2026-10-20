#include "House/HousePawn.h"
#include "Core/MansionInput.h"
#include "Core/MansionTuning.h"
#include "House/RevealMarker.h"
#include "Survivor/ExposureComponent.h"
#include "Survivor/SurvivorCharacter.h"
#include "World/GreyboxMansionBuilder.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "TimerManager.h"

namespace
{
	const FName RoofTag(TEXT("Roof"));

	/** Parses "Floor2" style tags; returns 0 if there is none. */
	int32 FindFloorTag(const TArray<FName>& Tags)
	{
		for (const FName& Tag : Tags)
		{
			const FString TagString = Tag.ToString();
			if (TagString.StartsWith(TEXT("Floor")))
			{
				return FCString::Atoi(*TagString.Mid(5));
			}
		}
		return 0;
	}

	FLinearColor ColorForReveal(const UExposureComponent& Exposure)
	{
		if (Exposure.HasRevealReason(ERevealReason::SeenByMonster))
		{
			return FLinearColor(1.f, 0.1f, 0.1f);
		}
		if (Exposure.HasRevealReason(ERevealReason::Dark))
		{
			return FLinearColor(0.6f, 0.2f, 1.f);
		}
		if (Exposure.HasRevealReason(ERevealReason::Camping))
		{
			return FLinearColor(1.f, 0.85f, 0.1f);
		}
		return FLinearColor::White;
	}
}

AHousePawn::AHousePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicatingMovement(false);

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(Pivot);
	CameraArm->SetRelativeRotation(FRotator(-65.f, 0.f, 0.f));
	CameraArm->TargetArmLength = 3000.f;
	CameraArm->bDoCollisionTest = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 8.f;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	// The mansion is meant to be dark; the overseer still needs to read the board.
	TopDownCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	TopDownCamera->PostProcessSettings.AutoExposureBias = 3.f;
}

void AHousePawn::BeginPlay()
{
	Super::BeginPlay();

	FloorHeight = FallbackFloorHeight;
	NumFloors = FallbackNumFloors;
	GroundZ = 0.f;

	for (TActorIterator<AGreyboxMansionBuilder> It(GetWorld()); It; ++It)
	{
		FloorHeight = It->GetLevelHeight();
		NumFloors = It->GetNumFloors();
		GroundZ = It->GetActorLocation().Z;
		break;
	}
}

void AHousePawn::PawnClientRestart()
{
	Super::PawnClientRestart();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
	}

	SetCurrentFloor(1);

	// Replicated actors (lights, monsters) keep arriving; re-apply the cutaway periodically.
	GetWorldTimerManager().SetTimer(CutawayRefreshTimer, this, &AHousePawn::ApplyFloorCutaway, 1.f, /*bLoop*/ true);
}

void AHousePawn::BuildInput()
{
	if (InputContext)
	{
		return;
	}

	InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_House"));
	PanAction = MansionInput::MakeAction(this, TEXT("IA_HousePan"), EInputActionValueType::Axis2D);
	ZoomAction = MansionInput::MakeAction(this, TEXT("IA_HouseZoom"), EInputActionValueType::Axis1D);
	FloorUpAction = MansionInput::MakeAction(this, TEXT("IA_HouseFloorUp"), EInputActionValueType::Boolean);
	FloorDownAction = MansionInput::MakeAction(this, TEXT("IA_HouseFloorDown"), EInputActionValueType::Boolean);

	MansionInput::MapWASD(InputContext, PanAction);
	MansionInput::MapKey(InputContext, ZoomAction, EKeys::MouseWheelAxis);
	MansionInput::MapKey(InputContext, FloorUpAction, EKeys::E);
	MansionInput::MapKey(InputContext, FloorDownAction, EKeys::Q);
}

void AHousePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	BuildInput();
	MansionInput::ApplyContext(this, InputContext);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(PanAction, ETriggerEvent::Triggered, this, &AHousePawn::Pan);
	Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AHousePawn::Zoom);
	Input->BindAction(FloorUpAction, ETriggerEvent::Started, this, &AHousePawn::FloorUp);
	Input->BindAction(FloorDownAction, ETriggerEvent::Started, this, &AHousePawn::FloorDown);
}

void AHousePawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateSurvivorMarkers();
	}
}

void AHousePawn::Pan(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawOnly(0.f, CameraArm->GetComponentRotation().Yaw, 0.f);
	const FVector Direction = YawOnly.RotateVector(FVector(Axis.Y, Axis.X, 0.f));
	const float Speed = CameraArm->TargetArmLength * PanSpeedFactor;
	AddActorWorldOffset(Direction * Speed * GetWorld()->GetDeltaSeconds());
}

void AHousePawn::Zoom(const FInputActionValue& Value)
{
	CameraArm->TargetArmLength = FMath::Clamp(CameraArm->TargetArmLength - Value.Get<float>() * ZoomStep, MinZoom, MaxZoom);
}

void AHousePawn::FloorUp()
{
	SetCurrentFloor(CurrentFloor + 1);
}

void AHousePawn::FloorDown()
{
	SetCurrentFloor(CurrentFloor - 1);
}

void AHousePawn::SetCurrentFloor(int32 NewFloor)
{
	CurrentFloor = FMath::Clamp(NewFloor, 1, FMath::Max(1, NumFloors));

	FVector Location = GetActorLocation();
	Location.Z = GetFloorBaseZ(CurrentFloor);
	SetActorLocation(Location);

	ApplyFloorCutaway();
}

float AHousePawn::GetFloorBaseZ(int32 Floor) const
{
	return GroundZ + (Floor - 1) * FloorHeight;
}

void AHousePawn::ApplyFloorCutaway()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !IsLocallyControlled())
	{
		return;
	}

	// Untagged things are cut by height: anything centred above the next floor's base.
	const float CutZ = GetFloorBaseZ(CurrentFloor + 1);

	PlayerController->HiddenPrimitiveComponents.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const AActor* Actor = *It;
		if (Actor == this || Actor->IsA<ASurvivorCharacter>())
		{
			continue;
		}

		const bool bActorIsRoof = Actor->ActorHasTag(RoofTag);
		const int32 ActorFloor = FindFloorTag(Actor->Tags);

		Actor->ForEachComponent<UPrimitiveComponent>(/*bIncludeFromChildActors*/ false, [&](UPrimitiveComponent* Component)
		{
			const int32 TaggedFloor = FMath::Max(FindFloorTag(Component->ComponentTags), ActorFloor);
			const bool bIsRoof = bActorIsRoof || Component->ComponentHasTag(RoofTag);

			const bool bHide = bIsRoof
				|| (TaggedFloor > 0 ? TaggedFloor > CurrentFloor : Component->Bounds.Origin.Z > CutZ);
			if (bHide)
			{
				PlayerController->HiddenPrimitiveComponents.Add(Component);
			}
		});
	}
}

void AHousePawn::UpdateSurvivorMarkers()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	const float GhostDuration = UMansionTuning::Get().RevealGhostDuration;

	for (TActorIterator<ASurvivorCharacter> It(GetWorld()); It; ++It)
	{
		ASurvivorCharacter* Survivor = *It;

		// The House never sees Survivors directly, nor the glow of their lights.
		PlayerController->HiddenActors.AddUnique(Survivor);
		Survivor->HideVisualLightsForHouseView();

		const UExposureComponent* Exposure = Survivor->GetExposure();
		TWeakObjectPtr<ARevealMarker>& Marker = Markers.FindOrAdd(Survivor);

		if (Exposure && Exposure->IsRevealed())
		{
			if (!Marker.IsValid())
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Marker = GetWorld()->SpawnActor<ARevealMarker>(Survivor->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
			}
			if (Marker.IsValid())
			{
				Marker->SetLive(Survivor->GetActorLocation(), ColorForReveal(*Exposure));
			}
		}
		else if (Marker.IsValid())
		{
			// Leave a fading ghost at the last known position; a new reveal spawns a fresh marker.
			Marker->StartGhost(GhostDuration);
			Marker.Reset();
		}
	}

	for (auto It = Markers.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			if (It.Value().IsValid())
			{
				It.Value()->Destroy();
			}
			It.RemoveCurrent();
		}
	}
}
