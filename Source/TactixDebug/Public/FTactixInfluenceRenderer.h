// Copyright Sleak Software. All Rights Reserved.
//
// FTactixInfluenceRenderer — draws a FTactixInfluenceMap channel as a grid of
// coloured world-space quads using debug draw primitives. Uses a blue→red heat
// gradient: zero = transparent, 1.0 = solid red. Cells below MinValue are
// skipped for performance.
//
// Typical usage: call once per frame from a debug console command or from
// UTactixGameplayDebuggerCategory::DrawData.

#pragma once

#include "CoreMinimal.h"
#include "Spatial/FTactixInfluenceMap.h"

class TACTIXDEBUG_API FTactixInfluenceRenderer
{
public:
	// Draw one channel of the influence map.
	// ZHeight     — world-space Z of the quad plane (float)
	// MinValue    — cells at or below this value are not drawn (perf guard)
	// QuadThickness — half-height of the debug box in UU
	static void DrawChannel(const UWorld* World,
	                        const Tactix::FTactixInfluenceMap<4>& Map,
	                        std::size_t Channel,
	                        float ZHeight       = 0.0f,
	                        float MinValue      = 0.01f,
	                        float QuadThickness = 5.0f);

	// Convenience: draw all 4 channels laid out side by side along the +Y axis,
	// separated by ChannelSpacing UU. Channel labels are drawn above each grid.
	static void DrawAllChannels(const UWorld* World,
	                            const Tactix::FTactixInfluenceMap<4>& Map,
	                            float ZHeight        = 0.0f,
	                            float ChannelSpacing = 500.0f);

private:
	// Linear interpolation from blue (0) to red (1) in [0, 1].
	static FColor HeatColor(float T);
};
