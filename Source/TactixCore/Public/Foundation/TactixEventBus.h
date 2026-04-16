// Copyright Sleak Software. All Rights Reserved.
//
// TactixEventBus — lightweight typed event channels.
//
// UE delegates are convenient but allocate on the heap for each bind and pull
// in UHT metadata we don't want in TactixCore. The event bus provides a minimal
// alternative: one `FTactixEventChannel<E>` per event type with a fixed-capacity
// array of `{function pointer, context pointer}` subscriber records.
//
// Semantics:
//   • Publish is synchronous and ordered by subscription registration.
//   • Subscribe returns false if the channel is full (compile-time MaxSubs).
//   • Unsubscribe matches on (Fn, Ctx) — the same pair passed to Subscribe.
//
// Typical usage:
//   struct FSquadTargetChanged { ... };
//   FTactixEventChannel<FSquadTargetChanged> OnTargetChanged;
//   OnTargetChanged.Subscribe(&OnTargetChangedCallback, this);
//   OnTargetChanged.Publish({ ... });

#pragma once

#include "TactixApi.h"

#include <cstddef>

namespace Tactix
{
	template <typename EventT, std::size_t MaxSubscribers = 16>
	class FTactixEventChannel
	{
	public:
		using HandlerFn = void (*)(const EventT&, void*);

		bool Subscribe(HandlerFn Fn, void* Ctx = nullptr)
		{
			if (Fn == nullptr) return false;
			if (Count >= MaxSubscribers) return false;
			Subs[Count++] = { Fn, Ctx };
			return true;
		}

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

		void Publish(const EventT& Event) const
		{
			for (std::size_t i = 0; i < Count; ++i)
			{
				Subs[i].Fn(Event, Subs[i].Ctx);
			}
		}

		void Clear() { Count = 0; }

		TACTIX_NODISCARD std::size_t NumSubscribers() const { return Count; }
		TACTIX_NODISCARD constexpr std::size_t MaxCapacity() const { return MaxSubscribers; }

	private:
		struct Subscription { HandlerFn Fn; void* Ctx; };
		Subscription Subs[MaxSubscribers]{};
		std::size_t  Count{0};
	};
}
