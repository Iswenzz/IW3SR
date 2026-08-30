#include "PMem.hpp"
#include "Dvar.hpp"
#include "Patch.hpp"

namespace IW3SR
{
	constexpr uintptr_t ReservationSize = 0x4FF23C;
	constexpr uintptr_t PrimTopSize = 0x4FF271;

	void GPMem::Initialize()
	{
		if (Patch::UseCoD4X)
			return;

		Apply(DefaultMegs);
	}

	void GPMem::InitDvars()
	{
		Com_InitDvars_h();

		if (MegsDvar)
			return;

		MegsDvar = Dvar::RegisterInt("sr_pmemMegs", DVAR_LATCHED,
			"Megabytes reserved for the physical memory block, read once at startup", DefaultMegs, MinMegs, MaxMegs);

		Apply(MegsDvar ? MegsDvar->current.integer : DefaultMegs);
		Log::WriteLine(Channel::System, "Reserving {} MB of physical memory.", Megs);
	}

	int GPMem::Reserved()
	{
		return Megs;
	}

	void GPMem::Apply(int megs)
	{
		Megs = std::clamp(megs, MinMegs, MaxMegs);

		const uint32_t bytes = static_cast<uint32_t>(Megs) * 1024u * 1024u;

		Memory::Set<uint32_t>(ReservationSize, bytes);
		Memory::Set<uint32_t>(PrimTopSize, bytes);
	}
}
