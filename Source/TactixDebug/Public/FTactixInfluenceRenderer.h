// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixInfluenceRenderer.h
 * @brief Draws an influence-map channel as a world-space heat grid.
 *
 * Each cell becomes a coloured debug quad on a cold-to-hot ramp (blue, cyan,
 * green, yellow, red): low values read blue, high values red. Cells at or below a
 * threshold are skipped so a mostly-empty map costs almost nothing to draw. Call
 * it once per frame from a console command or the Gameplay Debugger category.
 */

#pragma once

#include "CoreMinimal.h"
#include "Spatial/FTactixInfluenceMap.h"

/** @brief Namespace-like holder of influence-map debug-draw routines. */
class TACTIXDEBUG_API FTactixInfluenceRenderer
{
public:
	/**
	 * @brief Draws a single channel as a flat grid of heat-coloured quads.
	 * @param World         World to draw into.
	 * @param Map           Influence map to read.
	 * @param Channel       Channel index to visualise.
	 * @param ZHeight       World Z of the quad plane.
	 * @param MinValue      Cells at or below this are skipped (performance guard).
	 * @param QuadThickness Half-height of each debug box in UU.
	 */
	static void DrawChannel(const UWorld* World,
	                        const Tactix::FTactixInfluenceMap<4>& Map,
	                        std::size_t Channel,
	                        float ZHeight       = 0.0f,
	                        float MinValue      = 0.01f,
	                        float QuadThickness = 5.0f);

	/**
	 * @brief Draws all four channels side by side for comparison.
	 * @param World          World to draw into.
	 * @param Map            Influence map to read.
	 * @param ZHeight        World Z of the quad planes.
	 * @param ChannelSpacing Gap (UU) between channel grids along +Y. Each grid gets
	 *                       a label above it.
	 */
	static void DrawAllChannels(const UWorld* World,
	                            const Tactix::FTactixInfluenceMap<4>& Map,
	                            float ZHeight        = 0.0f,
	                            float ChannelSpacing = 500.0f);

private:
	/** @brief Maps a [0, 1] value to a cold-to-hot heat colour (blue through red). */
	static FColor HeatColor(float T);
};
