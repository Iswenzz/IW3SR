#include "PMem.hpp"
#include "Dvar.hpp"
#include "Patch.hpp"

namespace IW3SR
{
	constexpr uintptr_t ReservationSize = 0x4FF23C;
	constexpr uintptr_t PrimTopSize = 0x4FF271;

	// CoD4X's com_PMemMegs registration: max 600, min 128, then the default, 230 in 21.x.
	constexpr const char* CoD4XMegsSignature =
		"C7 44 24 0C 58 02 00 00 C7 44 24 08 80 00 00 00 C7 44 24 04 ?? ?? ?? ?? C7 04 24";
	constexpr uintptr_t CoD4XMegsDefault = 20;

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

	// CoD4X reserves the block itself from com_PMemMegs, read once at startup, so this
	// reservation never reaches it and its 230 MB default fails big maps with WIN_OUT_OF_MEM.
	// The default is rewritten as the DLL loads, before CoD4X registers the dvar;
	// +set com_PMemMegs still wins.
	void GPMem::RaiseCoD4X()
	{
		const std::vector<uintptr_t> sites = Signature::ScanAll(COD4X_BIN, CoD4XMegsSignature);
		for (const uintptr_t site : sites)
			Memory::Set<uint32_t>(site + CoD4XMegsDefault, static_cast<uint32_t>(DefaultMegs));

		if (sites.empty())
			Log::WriteLine(Channel::Error, "CoD4X's com_PMemMegs is not where {} puts it.", COD4X_BIN);
		else
			Log::WriteLine(Channel::System, "CoD4X reserves {} MB of physical memory.", DefaultMegs);
	}

	void GPMem::Apply(int megs)
	{
		Megs = std::clamp(megs, MinMegs, MaxMegs);

		const uint32_t bytes = static_cast<uint32_t>(Megs) * 1024u * 1024u;

		Memory::Set<uint32_t>(ReservationSize, bytes);
		Memory::Set<uint32_t>(PrimTopSize, bytes);
	}
}
