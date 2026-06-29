// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixInfluenceRenderer.cpp
 * @brief Implements the influence heat renderer with debug boxes.
 *
 * The heat ramp runs the full cold-to-hot spectrum (blue, cyan, green, yellow,
 * red) rather than a flat blue-to-red lerp, which reads more clearly at a glance.
 * The whole file collapses to empty stubs when @c ENABLE_DRAW_DEBUG is off.
 */

#include "FTactixInfluenceRenderer.h"
#include "DrawDebugHelpers.h"

#if ENABLE_DRAW_DEBUG

FColor FTactixInfluenceRenderer::HeatColor(float T)
{
	T = FMath::Clamp(T, 0.0f, 1.0f);
	// Blue (cold) → Cyan → Green → Yellow → Red (hot)
	if (T < 0.25f)
	{
		const float F = T / 0.25f;
		return FColor(0, static_cast<uint8>(F * 255), 255);             // Blue → Cyan
	}
	if (T < 0.5f)
	{
		const float F = (T - 0.25f) / 0.25f;
		return FColor(0, 255, static_cast<uint8>((1.0f - F) * 255));    // Cyan → Green
	}
	if (T < 0.75f)
	{
		const float F = (T - 0.5f) / 0.25f;
		return FColor(static_cast<uint8>(F * 255), 255, 0);             // Green → Yellow
	}
	{
		const float F = (T - 0.75f) / 0.25f;
		return FColor(255, static_cast<uint8>((1.0f - F) * 255), 0);   // Yellow → Red
	}
}

void FTactixInfluenceRenderer::DrawChannel(const UWorld* World,
                                            const Tactix::FTactixInfluenceMap<4>& Map,
                                            std::size_t Channel,
                                            float ZHeight,
                                            float MinValue,
                                            float QuadThickness)
{
	if (!World || Channel >= 4) return;

	const int32 W = Map.GetWidth();
	const int32 H = Map.GetHeight();
	const float HalfCell = Map.GetCellSize() * 0.5f;

	for (int32 y = 0; y < H; ++y)
	for (int32 x = 0; x < W; ++x)
	{
		const Tactix::FTactixVec2 Center = Map.CellCenter(x, y);
		const float Value = Map.SampleNearest(Channel, Center);
		if (Value <= MinValue) continue;

		const FColor Color = HeatColor(FMath::Clamp(Value, 0.0f, 1.0f));
		const FVector BoxCenter(Center.X, Center.Y, ZHeight);
		const FVector BoxExtent(HalfCell * 0.9f, HalfCell * 0.9f, QuadThickness);

		DrawDebugBox(World, BoxCenter, BoxExtent, FQuat::Identity, Color,
		             false, 0.0f, 0, 1.5f);
	}
}

void FTactixInfluenceRenderer::DrawAllChannels(const UWorld* World,
                                                const Tactix::FTactixInfluenceMap<4>& Map,
                                                float ZHeight,
                                                float ChannelSpacing)
{
	if (!World) return;

	static const TCHAR* ChannelNames[] = { TEXT("Danger"), TEXT("Control"), TEXT("Resource"), TEXT("Custom") };

	const float GridWidth = Map.GetWidth() * Map.GetCellSize();
	for (std::size_t C = 0; C < 4; ++C)
	{
		// Offset each channel along +Y so they don't overlap.
		// We temporarily shift the origin by offsetting our sample point.
		// Since the map's origin is fixed, we draw in the offset world space
		// and label it.
		const float YOffset = static_cast<float>(C) * (GridWidth + ChannelSpacing);

		const int32 W = Map.GetWidth();
		const int32 H = Map.GetHeight();
		const float HalfCell = Map.GetCellSize() * 0.5f;

		for (int32 y = 0; y < H; ++y)
		for (int32 x = 0; x < W; ++x)
		{
			const Tactix::FTactixVec2 MapCenter = Map.CellCenter(x, y);
			const float Value = Map.SampleNearest(C, MapCenter);
			if (Value <= 0.01f) continue;

			const FColor Color = HeatColor(FMath::Clamp(Value, 0.0f, 1.0f));
			const FVector BoxCenter(MapCenter.X, MapCenter.Y + YOffset, ZHeight);
			const FVector BoxExtent(HalfCell * 0.9f, HalfCell * 0.9f, 5.0f);

			DrawDebugBox(World, BoxCenter, BoxExtent, FQuat::Identity, Color,
			             false, 0.0f, 0, 1.5f);
		}

		// Channel label
		const Tactix::FTactixVec2 Origin = Map.GetOrigin();
		const FVector LabelPos(Origin.X, Origin.Y + YOffset - 100.0f, ZHeight + 40.0f);
		DrawDebugString(World, LabelPos, ChannelNames[C], nullptr, FColor::White,
		                0.0f, true, 1.2f);
	}
}

#else // ENABLE_DRAW_DEBUG

void FTactixInfluenceRenderer::DrawChannel(const UWorld*, const Tactix::FTactixInfluenceMap<4>&,
                                            std::size_t, float, float, float) {}
void FTactixInfluenceRenderer::DrawAllChannels(const UWorld*, const Tactix::FTactixInfluenceMap<4>&,
                                                float, float) {}
FColor FTactixInfluenceRenderer::HeatColor(float) { return FColor::White; }

#endif // ENABLE_DRAW_DEBUG
