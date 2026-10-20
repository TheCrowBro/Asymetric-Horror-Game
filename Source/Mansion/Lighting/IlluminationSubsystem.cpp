#include "Lighting/IlluminationSubsystem.h"
#include "Lighting/GameplayLightComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

static TAutoConsoleVariable<bool> CVarShowLights(
	TEXT("mansion.ShowLights"),
	false,
	TEXT("Draw gameplay light radii (yellow = on, grey = off) and room bounds."),
	ECVF_Cheat);

bool UIlluminationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UIlluminationSubsystem::Tick(float DeltaTime)
{
	if (!CVarShowLights.GetValueOnGameThread())
	{
		return;
	}

	const UWorld* World = GetWorld();
	for (const TWeakObjectPtr<UGameplayLightComponent>& WeakLight : Lights)
	{
		const UGameplayLightComponent* Light = WeakLight.Get();
		if (!Light)
		{
			continue;
		}
		const FColor Color = Light->IsLightEnabled() ? FColor::Yellow : FColor(80, 80, 80);
		if (Light->bSpot)
		{
			const float HalfAngle = FMath::DegreesToRadians(Light->OuterConeAngle);
			DrawDebugCone(World, Light->GetComponentLocation(), Light->GetForwardVector(), Light->Radius, HalfAngle, HalfAngle, 12, Color);
		}
		else
		{
			DrawDebugSphere(World, Light->GetComponentLocation(), Light->Radius, 16, Color);
		}
	}
	for (const TPair<int32, FMansionRoom>& Pair : Rooms)
	{
		DrawDebugBox(World, Pair.Value.Bounds.GetCenter(), Pair.Value.Bounds.GetExtent(), FColor::Cyan);
	}
}

void UIlluminationSubsystem::RegisterLight(UGameplayLightComponent* Light)
{
	Lights.AddUnique(Light);
}

void UIlluminationSubsystem::UnregisterLight(UGameplayLightComponent* Light)
{
	Lights.Remove(Light);
}

int32 UIlluminationSubsystem::RegisterRoom(const FMansionRoom& Room)
{
	const int32 RoomId = NextRoomId++;
	Rooms.Add(RoomId, Room);
	return RoomId;
}

void UIlluminationSubsystem::UnregisterRoom(int32 RoomId)
{
	Rooms.Remove(RoomId);
}

const FMansionRoom* UIlluminationSubsystem::FindRoomAt(const FVector& Location) const
{
	for (const TPair<int32, FMansionRoom>& Pair : Rooms)
	{
		if (Pair.Value.Bounds.IsInsideOrOn(Location))
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

float UIlluminationSubsystem::GetIlluminationAt(const FVector& Location, const AActor* QueryActor) const
{
	float Total = 0.f;
	if (const FMansionRoom* Room = FindRoomAt(Location))
	{
		Total += Room->AmbientLight;
	}

	const UWorld* World = GetWorld();
	for (const TWeakObjectPtr<UGameplayLightComponent>& WeakLight : Lights)
	{
		const UGameplayLightComponent* Light = WeakLight.Get();
		if (!Light || !Light->IsLightEnabled())
		{
			continue;
		}

		const AActor* LightOwner = Light->GetOwner();
		if (QueryActor && LightOwner == QueryActor)
		{
			Total += Light->Intensity * Light->SelfIlluminationFactor;
			continue;
		}

		const FVector LightLocation = Light->GetComponentLocation();
		const FVector ToTarget = Location - LightLocation;
		float Contribution = ComputeFalloff(Light->Intensity, Light->Radius, ToTarget.Size());
		if (Light->bSpot)
		{
			Contribution *= ComputeConeFactor(Light->GetForwardVector(), ToTarget.GetSafeNormal(), Light->InnerConeAngle, Light->OuterConeAngle);
		}
		if (Contribution <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(MansionIllumination), /*bTraceComplex*/ false);
		Params.AddIgnoredActor(LightOwner);
		if (QueryActor)
		{
			Params.AddIgnoredActor(QueryActor);
		}
		if (World->LineTraceTestByChannel(LightLocation, Location, ECC_Visibility, Params))
		{
			continue; // Occluded.
		}

		Total += Contribution;
	}
	return Total;
}

float UIlluminationSubsystem::ComputeFalloff(float Intensity, float Radius, float Distance)
{
	if (Radius <= 0.f || Distance >= Radius)
	{
		return 0.f;
	}
	const float Alpha = 1.f - FMath::Max(Distance, 0.f) / Radius;
	return Intensity * Alpha * Alpha;
}

float UIlluminationSubsystem::ComputeConeFactor(const FVector& LightForward, const FVector& DirectionToTarget, float InnerConeDegrees, float OuterConeDegrees)
{
	const float CosAngle = FMath::Clamp(FVector::DotProduct(LightForward.GetSafeNormal(), DirectionToTarget.GetSafeNormal()), -1.f, 1.f);
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(CosAngle));

	if (AngleDegrees <= InnerConeDegrees)
	{
		return 1.f;
	}
	if (AngleDegrees >= OuterConeDegrees || OuterConeDegrees <= InnerConeDegrees)
	{
		return 0.f;
	}
	return 1.f - (AngleDegrees - InnerConeDegrees) / (OuterConeDegrees - InnerConeDegrees);
}
