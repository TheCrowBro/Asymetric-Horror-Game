#include "Survivor/SurvivorCharacter.h"
#include "Core/MansionGameState.h"
#include "Core/MansionInput.h"
#include "Core/MansionTuning.h"
#include "Lighting/GameplayLightComponent.h"
#include "Survivor/ExposureComponent.h"
#include "Survivor/FearComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ASurvivorCharacter::ASurvivorCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	BaseEyeHeight = 64.f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	GreyboxBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreyboxBody"));
	GreyboxBody->SetupAttachment(GetCapsuleComponent());
	GreyboxBody->SetStaticMesh(CylinderMesh.Object);
	GreyboxBody->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.76f));
	GreyboxBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GreyboxBody->SetOwnerNoSee(true);

	FlashlightLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightLight"));
	FlashlightLight->SetupAttachment(FirstPersonCamera);
	FlashlightLight->SetRelativeLocation(FVector(20.f, 15.f, -15.f));
	FlashlightLight->SetInnerConeAngle(12.f);
	FlashlightLight->SetOuterConeAngle(25.f);
	FlashlightLight->SetAttenuationRadius(1500.f);
	FlashlightLight->SetVisibility(false);

	FlashlightGameplayLight = CreateDefaultSubobject<UGameplayLightComponent>(TEXT("FlashlightGameplayLight"));
	FlashlightGameplayLight->SetupAttachment(FlashlightLight);
	FlashlightGameplayLight->bSpot = true;
	FlashlightGameplayLight->Intensity = 0.8f;
	FlashlightGameplayLight->Radius = 1500.f;
	FlashlightGameplayLight->InnerConeAngle = 12.f;
	FlashlightGameplayLight->OuterConeAngle = 25.f;
	FlashlightGameplayLight->SelfIlluminationFactor = 0.5f;

	Exposure = CreateDefaultSubobject<UExposureComponent>(TEXT("Exposure"));
	Fear = CreateDefaultSubobject<UFearComponent>(TEXT("Fear"));
}

void ASurvivorCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The owner applies sprint locally for responsiveness.
	DOREPLIFETIME_CONDITION(ASurvivorCharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME(ASurvivorCharacter, bFlashlightOn);
}

void ASurvivorCharacter::BeginPlay()
{
	Super::BeginPlay();

	FlashlightGameplayLight->SetLinkedLight(FlashlightLight);
	ApplyMovementSpeed();
	ApplyFlashlightState();
}

void ASurvivorCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void ASurvivorCharacter::BuildInput()
{
	if (InputContext)
	{
		return;
	}

	InputContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Survivor"));
	MoveAction = MansionInput::MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MansionInput::MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	SprintAction = MansionInput::MakeAction(this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	CrouchAction = MansionInput::MakeAction(this, TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	FlashlightAction = MansionInput::MakeAction(this, TEXT("IA_Flashlight"), EInputActionValueType::Boolean);

	MansionInput::MapWASD(InputContext, MoveAction);
	MansionInput::MapKey(InputContext, LookAction, EKeys::Mouse2D, false, false, /*bNegateY*/ true);
	MansionInput::MapKey(InputContext, SprintAction, EKeys::LeftShift);
	MansionInput::MapKey(InputContext, CrouchAction, EKeys::LeftControl);
	MansionInput::MapKey(InputContext, CrouchAction, EKeys::C);
	MansionInput::MapKey(InputContext, FlashlightAction, EKeys::F);
}

void ASurvivorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	BuildInput();
	MansionInput::ApplyContext(this, InputContext);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASurvivorCharacter::Look);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ASurvivorCharacter::StartSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASurvivorCharacter::StopSprint);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASurvivorCharacter::ToggleCrouch);
	Input->BindAction(FlashlightAction, ETriggerEvent::Started, this, &ASurvivorCharacter::ToggleFlashlight);
}

bool ASurvivorCharacter::CanAct() const
{
	const AMansionGameState* GameState = GetWorld()->GetGameState<AMansionGameState>();
	if (!GameState)
	{
		return true;
	}
	const EMansionPhase Phase = GameState->GetPhase();
	return Phase != EMansionPhase::PreGame && Phase != EMansionPhase::Ended;
}

void ASurvivorCharacter::Move(const FInputActionValue& Value)
{
	if (!CanAct())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void ASurvivorCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void ASurvivorCharacter::StartSprint()
{
	SetSprinting(true);
}

void ASurvivorCharacter::StopSprint()
{
	SetSprinting(false);
}

void ASurvivorCharacter::ToggleCrouch()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else if (CanAct())
	{
		Crouch();
	}
}

void ASurvivorCharacter::ToggleFlashlight()
{
	SetFlashlightOn(!bFlashlightOn);
}

void ASurvivorCharacter::SetSprinting(bool bSprinting)
{
	bIsSprinting = bSprinting;
	ApplyMovementSpeed();

	if (!HasAuthority())
	{
		ServerSetSprinting(bSprinting);
	}
}

void ASurvivorCharacter::ServerSetSprinting_Implementation(bool bSprinting)
{
	bIsSprinting = bSprinting;
	ApplyMovementSpeed();
}

void ASurvivorCharacter::OnRep_IsSprinting()
{
	ApplyMovementSpeed();
}

void ASurvivorCharacter::ApplyMovementSpeed()
{
	const UMansionTuning& Tuning = UMansionTuning::Get();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = bIsSprinting ? Tuning.SprintSpeed : Tuning.WalkSpeed;
	Movement->MaxWalkSpeedCrouched = Tuning.CrouchSpeed;
}

void ASurvivorCharacter::SetFlashlightOn(bool bOn)
{
	if (HasAuthority())
	{
		bFlashlightOn = bOn;
		ApplyFlashlightState();
	}
	else
	{
		ServerSetFlashlightOn(bOn);
	}
}

void ASurvivorCharacter::ServerSetFlashlightOn_Implementation(bool bOn)
{
	SetFlashlightOn(bOn);
}

void ASurvivorCharacter::OnRep_FlashlightOn()
{
	ApplyFlashlightState();
}

void ASurvivorCharacter::ApplyFlashlightState()
{
	FlashlightGameplayLight->SetLightEnabled(bFlashlightOn);
}

void ASurvivorCharacter::HideVisualLightsForHouseView()
{
	FlashlightLight->SetVisibility(false);
}
