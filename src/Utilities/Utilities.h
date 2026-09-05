#pragma once

#include <Windows.h>

#ifdef ERROR
#	undef ERROR  // wingdi.h defines this and it swallows REX::ERROR
#endif

namespace Utilities
{
	bool InitializeActorInstant(RE::Actor& a_actor, bool a_update3D);
	RE::TESForm* GetFormFromMod(std::string modname, uint32_t formid);
	uintptr_t GetFallout4BaseAddress();

	template <class FunctionPointer>
	FunctionPointer SafeWrite64Function(uintptr_t addr, FunctionPointer data)
	{
		DWORD oldProtect;
		void* _d[2];
		memcpy(_d, &data, sizeof(data));
		size_t len = sizeof(_d[0]);

		VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
		FunctionPointer olddata;
		memset(&olddata, 0, sizeof(FunctionPointer));
		memcpy(&olddata, (void*)addr, len);
		memcpy((void*)addr, &_d[0], len);
		VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
		return olddata;
	}

	// -------------------------------------------------------------------------
	// Signature scanning
	//
	// REL already ships REL::Pattern, but that is only a *matcher*: it validates
	// the bytes sitting at an address you already know. What follows adds the
	// missing half, walking a module section to find where a compile time pattern
	// actually lands, so a hook can be anchored to the code around it instead of
	// to a hardcoded offset that has to be redone on every game update.
	// -------------------------------------------------------------------------

	struct MemoryRange
	{
		uintptr_t base{ 0 };  // module base, so hits can be reported as module+offset
		uintptr_t begin{ 0 };
		size_t    size{ 0 };

		[[nodiscard]] constexpr bool      Empty() const noexcept { return size == 0; }
		[[nodiscard]] constexpr uintptr_t End() const noexcept { return begin + size; }
		[[nodiscard]] constexpr uintptr_t OffsetOf(uintptr_t a_address) const noexcept { return a_address - base; }
	};

	// The .text section of Fallout4.exe.
	[[nodiscard]] MemoryRange GetGameCodeRange();

	namespace detail
	{
		[[nodiscard]] consteval int HexDigit(char a_char) noexcept
		{
			if (a_char >= '0' && a_char <= '9') {
				return a_char - '0';
			}

			if (a_char >= 'a' && a_char <= 'f') {
				return a_char - 'a' + 0xA;
			}

			if (a_char >= 'A' && a_char <= 'F') {
				return a_char - 'A' + 0xA;
			}

			return -1;
		}
	}

	// "41 B8 ?? 00" -> 11 characters, 4 bytes.
	template <REX::TStaticString S>
	inline constexpr size_t PatternTextLength = S.length();

	template <REX::TStaticString S>
	inline constexpr size_t PatternSize = (PatternTextLength<S> + 1) / 3;

	template <REX::TStaticString S>
	[[nodiscard]] constexpr std::string_view PatternText() noexcept
	{
		return std::string_view{ S.c, PatternTextLength<S> };
	}

	// Leading byte of the pattern, or -1 when it opens on a wildcard.
	template <REX::TStaticString S>
	inline constexpr int PatternLeadByte =
		(PatternTextLength<S> >= 2 && detail::HexDigit(S.value_at(0)) >= 0 && detail::HexDigit(S.value_at(1)) >= 0) ?
			detail::HexDigit(S.value_at(0)) * 0x10 + detail::HexDigit(S.value_at(1)) :
			-1;

	template <REX::TStaticString S>
	inline constexpr bool PatternEndsOnCall =
		PatternTextLength<S> >= 2 &&
		S.value_at(PatternTextLength<S> - 2) == 'E' &&
		S.value_at(PatternTextLength<S> - 1) == '8';

	// Every address in a_range whose bytes satisfy S.
	template <REX::TStaticString S>
	[[nodiscard]] std::vector<uintptr_t> FindPatternMatches(const MemoryRange& a_range)
	{
		constexpr auto matcher = REL::Pattern<S>();
		constexpr auto size = PatternSize<S>;
		constexpr auto lead = PatternLeadByte<S>;

		std::vector<uintptr_t> matches;

		if (a_range.Empty() || a_range.size < size) {
			return matches;
		}

		const auto last = a_range.End() - size;  // last address the pattern still fits at
		auto       cursor = a_range.begin;

		while (cursor <= last) {
			if constexpr (lead >= 0) {
				// Skip straight to the next candidate instead of testing every byte,
				// .text is roughly 37MB and this runs on every game launch.
				const auto hit = std::memchr(reinterpret_cast<const void*>(cursor), lead, (last - cursor) + 1);

				if (!hit) {
					break;
				}

				cursor = reinterpret_cast<uintptr_t>(hit);
			}

			if (matcher.match(cursor)) {
				matches.push_back(cursor);
			}

			++cursor;
		}

		return matches;
	}

	// Resolves S to exactly one address. Both "no hit" and "several hits" count as
	// a failure, because either one means the pattern no longer pins down the code
	// we are after on this build of the game. Returns 0 in that case.
	template <REX::TStaticString S>
	[[nodiscard]] uintptr_t FindPattern(const MemoryRange& a_range)
	{
		const auto matches = FindPatternMatches<S>(a_range);

		if (matches.size() == 1) {
			return matches.front();
		}

		if (matches.empty()) {
			REX::ERROR("Pattern \"{}\" did not match anything.", PatternText<S>());
		} else {
			REX::ERROR("Pattern \"{}\" matched {} times, it no longer identifies a single site.", PatternText<S>(), matches.size());
		}

		return 0;
	}

	// For patterns that end on the E8 of the call we want to redirect: returns the
	// address of that call instruction rather than the start of the match.
	template <REX::TStaticString S>
	[[nodiscard]] uintptr_t FindTrailingCall(const MemoryRange& a_range)
	{
		static_assert(PatternEndsOnCall<S>, "pattern must end on the E8 of the call instruction it targets");

		const auto match = FindPattern<S>(a_range);
		return match ? match + PatternSize<S> - 1 : 0;
	}
}
