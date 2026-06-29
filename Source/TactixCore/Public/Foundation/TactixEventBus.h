// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixEventBus.h
 * @brief Lightweight typed event channel with no heap traffic per publish.
 *
 * UE delegates are convenient, but each bind allocates and they drag UHT
 * reflection metadata into a module that is supposed to stay engine-free. This
 * is the cheap alternative: one @ref Tactix::FTactixEventChannel "FTactixEventChannel" per event type, holding
 * a fixed-size array of `{function pointer, context pointer}` records. Binding,
 * unbinding and publishing all touch only that inline array.
 *
 * The contract in short:
 *  - publish is synchronous and runs handlers in subscription order;
 *  - subscribe fails (returns false) once the compile-time capacity is reached;
 *  - unsubscribe matches on the exact `(Fn, Ctx)` pair you subscribed with.
 *
 * @code
 * struct FSquadTargetChanged { FTactixHandle<ITactixAgent> NewTarget; };
 *
 * FTactixEventChannel<FSquadTargetChanged> OnTargetChanged;
 * OnTargetChanged.Subscribe(&HandleTargetChanged, this);
 * OnTargetChanged.Publish({ NewTarget });
 * @endcode
 */

#pragma once

#include "TactixApi.h"

#include <cstddef>

namespace Tactix
{
	/**
	 * @brief A single-event publish/subscribe channel with fixed capacity.
	 * @tparam EventT         Payload type, passed to handlers by const reference.
	 * @tparam MaxSubscribers Largest number of concurrent subscribers. Past this,
	 *                        @ref Subscribe starts refusing.
	 */
	template <typename EventT, std::size_t MaxSubscribers = 16>
	class FTactixEventChannel
	{
	public:
		/**
		 * @brief Handler signature. The @c void* is the context given to @ref Subscribe.
		 *
		 * A free function or `static` method is required, not a capturing lambda,
		 * which is the price of staying allocation-free. Carry per-instance state
		 * through the context pointer (typically @c this).
		 */
		using HandlerFn = void (*)(const EventT&, void*);

		/**
		 * @brief Registers a handler.
		 * @param Fn  Handler to call on publish. A null @p Fn is rejected.
		 * @param Ctx Opaque context handed back to @p Fn unchanged.
		 * @return True if registered; false if @p Fn was null or the channel is full.
		 * @note Subscribing the same `(Fn, Ctx)` twice registers it twice, so it
		 *       will fire twice. Unsubscribe removes one of them.
		 */
		bool Subscribe(HandlerFn Fn, void* Ctx = nullptr)
		{
			if (Fn == nullptr) return false;
			if (Count >= MaxSubscribers) return false;
			Subs[Count++] = { Fn, Ctx };
			return true;
		}

		/**
		 * @brief Removes the first subscriber matching both @p Fn and @p Ctx.
		 * @param Fn  Handler originally passed to @ref Subscribe.
		 * @param Ctx Context originally passed to @ref Subscribe.
		 * @return True if a match was found and removed.
		 * @note Removal swaps the last entry into the gap, so subscription order is
		 *       not preserved after an unsubscribe.
		 */
		bool Unsubscribe(HandlerFn Fn, void* Ctx)
		{
			for (std::size_t i = 0; i < Count; ++i)
			{
				if (Subs[i].Fn == Fn && Subs[i].Ctx == Ctx)
				{
					Subs[i] = Subs[--Count];
					return true;
				}
			}
			return false;
		}

		/**
		 * @brief Calls every subscriber with @p Event, in current array order.
		 * @param Event Payload, forwarded by const reference to each handler.
		 * @warning Handlers run synchronously. Subscribing or unsubscribing from
		 *          inside a handler mutates the array mid-iteration and is not safe.
		 */
		void Publish(const EventT& Event) const
		{
			for (std::size_t i = 0; i < Count; ++i)
			{
				Subs[i].Fn(Event, Subs[i].Ctx);
			}
		}

		/** @brief Drops all subscribers. */
		void Clear() { Count = 0; }

		/** @brief Current subscriber count. */
		TACTIX_NODISCARD std::size_t NumSubscribers() const { return Count; }
		/** @brief Compile-time subscriber ceiling (@p MaxSubscribers). */
		TACTIX_NODISCARD constexpr std::size_t MaxCapacity() const { return MaxSubscribers; }

	private:
		/** @brief One bound handler and the context it was bound with. */
		struct Subscription { HandlerFn Fn; void* Ctx; };
		Subscription Subs[MaxSubscribers]{};  ///< Inline subscriber records, no heap.
		std::size_t  Count{0};                ///< Number of slots in use.
	};
}
