#include "Materials.hpp"

namespace IW3SR
{
	constexpr int StockMaterials = 2048;

	constexpr uintptr_t StockSortedMaterials = 0xCC98280;
	constexpr uintptr_t StockMaterialHashTable = 0xCC9D2B4;

	// The predicate R_SortMaterials hands to std::sort, and the call R_ClearGlobals makes to wipe
	// the back half of r_global_permanent_t.
	constexpr uintptr_t SortPredicateSite = 0x621610;
	constexpr uintptr_t StockSortPredicate = 0x621250;
	constexpr uintptr_t ClearGlobalsSite = 0x5F4E1F;
	constexpr uintptr_t StockMemset = 0x67C4A0;


	// One rewritten operand: where it sits, how wide it is, and the value the stock image must
	// still hold there.
	struct MaterialPatch
	{
		uintptr_t address;
		int width;
		uint32_t expected;
		uint32_t value;
	};

	namespace
	{
		Material* SortedMaterials[GMaterials::MaxMaterials];
		Material* MaterialHashTable[GMaterials::MaxMaterials];

		constexpr uintptr_t SortedSites[] = { 0x5F27C6, 0x5F97F2, 0x6029D8, 0x6029F2, 0x621722,
			0x62173C, 0x648E1B, 0x648E74, 0x64900F };

		constexpr uintptr_t HashSites[] = { 0x5F27DE, 0x5F282F, 0x5F2987, 0x5F2997, 0x5F29E3,
			0x5F2A2A, 0x5F2B12, 0x5F2BAE, 0x5F2D32 };

		constexpr MaterialPatch Limits[] = {
			// Material_Register's "the list is full" warning.
			{ 0x5F27D6, 4, 0x800, GMaterials::MaxMaterials },

			// The capacity both R_UpdateMaterialSortList callers hand DB_EnumXAssets. It ignores
			// the argument entirely, which is why the list overran in the first place; these are
			// corrected so the call no longer misdescribes the array.
			{ 0x6029D3, 4, 0x800, GMaterials::MaxMaterials },
			{ 0x62171D, 4, 0x800, GMaterials::MaxMaterials },

			// The hash table is walked and cleared by byte count.
			{ 0x5F2B26, 4, 0x2000, 4 * GMaterials::MaxMaterials },
			{ 0x5F2BA7, 4, 0x2000, 4 * GMaterials::MaxMaterials },

			// Material_HashName ends in a magic-number division by 2047. All three constants
			// belong to the divisor, not just the imul: the pair below is the exact magic and
			// shift for 4095, checked at every quotient boundary over the whole 32 bit range.
			{ 0x5F21E7, 4, 0x200401, 0x100101 },
			{ 0x5F21F7, 1, 0x0A, 0x0B },
			{ 0x5F21FC, 4, 0x7FF, GMaterials::MaxMaterials - 1 },

			// The probe step wraps on the same divisor.
			{ 0x5F29CF, 4, 0x7FF, GMaterials::MaxMaterials - 1 },
		};

		bool HasDrawTechnique(const MaterialTechniqueSet* set)
		{
			if (!set)
				return false;

			for (int type = 0; type < std::size(set->techniques); type++)
			{
				if (type != TECHNIQUE_UNLIT && set->techniques[type])
					return true;
			}
			return false;
		}

		// Only a material with nothing but an unlit technique is 2D. cameraRegion is not enough:
		// the shadow map and dynamic light passes build draw surfaces from any material carrying
		// their technique, whatever its region.
		bool IsFlat(const Material* material)
		{
			const MaterialTechniqueSet* set = material->techniqueSet;
			if (!set)
				return false;

			return !HasDrawTechnique(set) && !HasDrawTechnique(set->remappedTechniqueSet);
		}

		uint32_t Read(uintptr_t address, int width)
		{
			return width == 1 ? Memory::Get<uint8_t>(address) : Memory::Get<uint32_t>(address);
		}

		bool Expect(uintptr_t address, uint32_t expected, int width)
		{
			const uint32_t value = Read(address, width);
			if (value == expected)
				return true;

			Log::WriteLine(Channel::Error,
				"Material limit: {:#x} holds {:#x}, expected {:#x}; keeping the stock limit.", address, value,
				expected);
			return false;
		}
	}

	void GMaterials::Initialize()
	{
		if (Relocated || !Verify())
			return;

		const auto sorted = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(SortedMaterials));
		const auto hash = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(MaterialHashTable));

		for (const uintptr_t site : SortedSites)
			Memory::Set<uint32_t>(site, sorted);

		for (const uintptr_t site : HashSites)
			Memory::Set<uint32_t>(site, hash);

		for (const MaterialPatch& patch : Limits)
		{
			if (patch.width == 1)
				Memory::Set<uint8_t>(patch.address, static_cast<uint8_t>(patch.value));
			else
				Memory::Set<uint32_t>(patch.address, patch.value);
		}

		Memory::Set<uint32_t>(SortPredicateSite,
			static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&GMaterials::Sort)));
		Memory::CALL(ClearGlobalsSite, reinterpret_cast<uintptr_t>(&GMaterials::Clear));

		Relocated = true;
	}

	int GMaterials::PoolSize()
	{
		return Relocated ? MaxMaterials : StockMaterials;
	}

	Material** GMaterials::Sorted()
	{
		return Relocated ? SortedMaterials : rgp->sortedMaterials;
	}

	// Nothing can be done about this from here short of widening the 11 bit index, but a map that
	// silently draws the wrong materials is worth naming.
	void GMaterials::WarnOnOverflow()
	{
		static int warned = 0;

		Material** const sorted = Sorted();
		const int count = std::min(rgp->materialCount, MaxMaterials);
		int drawable = 0;

		for (int i = 0; i < count; i++)
		{
			const Material* material = sorted[i];
			if (material && !IsFlat(material))
				drawable++;
		}

		if (drawable <= MaxDrawableMaterials || drawable == warned)
			return;
		warned = drawable;

		Log::WriteLine(Channel::Warning,
			"{} materials can reach a draw surface but only {} fit its index; the rest will draw as the "
			"wrong material.",
			drawable, MaxDrawableMaterials);
	}

	// Every operand has to still read what the stock image put there. One surprise means something
	// else has already rewritten the renderer, and half a relocation is worse than none.
	bool GMaterials::Verify()
	{
		bool ok = true;

		for (const uintptr_t site : SortedSites)
			ok &= Expect(site, StockSortedMaterials, 4);

		for (const uintptr_t site : HashSites)
			ok &= Expect(site, StockMaterialHashTable, 4);

		for (const MaterialPatch& patch : Limits)
			ok &= Expect(patch.address, patch.expected, patch.width);

		ok &= Expect(SortPredicateSite, StockSortPredicate, 4);
		ok &= Expect(ClearGlobalsSite, 0xE8, 1);
		ok &= Expect(ClearGlobalsSite + 1, StockMemset - (ClearGlobalsSite + 5), 4);

		return ok;
	}

	// Materials that never reach a draw surface sort last, so the ones that do keep the low indices
	// that the surface's 11 bit field can name. Order within each group is the engine's own.
	bool GMaterials::Sort(Material* a, Material* b)
	{
		const bool flat = IsFlat(a);
		if (flat != IsFlat(b))
			return !flat;

		return Material_SortLess(a, b);
	}

	// R_ClearGlobals wipes r_global_permanent_t from the material list onwards. The relocated
	// arrays are no longer in that range, so they are wiped alongside it.
	void* GMaterials::Clear(void* memory, int value, size_t size)
	{
		std::memset(SortedMaterials, 0, sizeof(SortedMaterials));
		std::memset(MaterialHashTable, 0, sizeof(MaterialHashTable));

		return std::memset(memory, value, size);
	}
}
