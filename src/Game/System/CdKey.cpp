#include "CdKey.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

#include <cstring>

// cdkey1..cdkey5 hold the fragments of the player's CD key and a server can make the client echo a
// dvar back, so they are wiped before the connect handshake leaves and kept wiped while connected.
// Nothing is lost: the key lives in the registry, and the key entry menu refills the fragments.

namespace IW3SR
{
	static const char* const KeyDvars[] = { "cdkey1", "cdkey2", "cdkey3", "cdkey4", "cdkey5" };

	constexpr DWORD PageWritable = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

	void GCdKey::Initialize()
	{
		EnabledDvar = Dvar::RegisterBool("sr_cdkey_protect", DVAR_SAVED,
			"Wipe the cdkey dvars before connecting so a server cannot read the CD key back", true);
	}

	void GCdKey::Protect()
	{
		// A missing Initialize() would otherwise leave the key exposed in complete silence.
		if (!EnabledDvar && !Unwired)
		{
			Unwired = true;
			Log::WriteLine(Channel::Error,
				"GCdKey::Initialize() was never called, so the CD key is NOT being wiped before connecting.");
		}

		if (!IsEnabled())
			return;

		Resolve();

		int cleared = 0;
		for (const dvar_s* dvar : Fragments)
			cleared += Blank(dvar);

		if (cleared)
			Log::WriteLine(Channel::Debug, "Wiped {} cdkey fragment(s) before connecting.", cleared);

		// Retail keeps cl_cdkey in a buffer filled from the registry, not in a dvar, and the connect
		// GUID is an MD5 of that buffer, so clearing it would mint a new identity.
		if (!Warned && Dvar::Find("cl_cdkey"))
		{
			Warned = true;
			Log::WriteLine(Channel::Warning,
				"cl_cdkey exists as a dvar. IW3SR leaves it alone, clearing it would change the player's GUID.");
		}
	}

	// A server can stuff the key entry menu open to make the engine refill the fragments after the
	// handshake. Connected states only, so opening the menu by hand from the main menu still works.
	void GCdKey::Frame()
	{
		if (!IsEnabled() || !client_ui || client_ui->connectionState < CA_CONNECTING)
			return;

		Resolve();

		for (const dvar_s* dvar : Fragments)
			Blank(dvar);
	}

	// Looked up once: the engine's pool keeps the pointers stable, and Dvar::Find builds a
	// std::string per call — five allocations every frame otherwise.
	void GCdKey::Resolve()
	{
		for (size_t i = 0; i < std::size(KeyDvars); ++i)
		{
			if (!Fragments[i])
				Fragments[i] = Dvar::Find(KeyDvars[i]);
		}
	}

	// CoD4X blanks the same five dvars from its own connect path.
	bool GCdKey::IsEnabled()
	{
		return !Patch::UseCoD4X && EnabledDvar && EnabledDvar->current.enabled;
	}

	// current, latched and reset normally share one allocation, so each distinct pointer is wiped
	// once. The reset value matters too: a "reset cdkey1" would otherwise hand the fragment back.
	bool GCdKey::Blank(const dvar_s* dvar)
	{
		if (!dvar || dvar->type != DvarType::STRING)
			return false;

		bool cleared = Wipe(dvar->current.string);

		if (dvar->latched.string != dvar->current.string && Wipe(dvar->latched.string))
			cleared = true;
		if (dvar->reset.string != dvar->current.string && dvar->reset.string != dvar->latched.string
			&& Wipe(dvar->reset.string))
			cleared = true;

		return cleared;
	}

	// Zeroed where it lies rather than repointed: no pointer changes hands, so the engine's own free
	// path keeps working and no copy of the key is left behind.
	bool GCdKey::Wipe(const char* string)
	{
		if (!string || !*string)
			return false;

		const size_t length = std::strlen(string);
		if (!Writable(string, length))
			return false;

		std::memset(const_cast<char*>(string), 0, length);
		return true;
	}

	bool GCdKey::Writable(const void* address, size_t size)
	{
		MEMORY_BASIC_INFORMATION info = {};
		if (VirtualQuery(address, &info, sizeof(info)) != sizeof(info) || info.State != MEM_COMMIT)
			return false;
		if ((info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) || !(info.Protect & PageWritable))
			return false;

		// A span reaching past the region VirtualQuery described is not worth a second guess.
		const auto start = static_cast<const uint8_t*>(address);
		const auto end = static_cast<const uint8_t*>(info.BaseAddress) + info.RegionSize;
		return start + size <= end;
	}
}
