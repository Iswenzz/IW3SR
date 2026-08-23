#include "Server.hpp"

namespace IW3SR
{
	gentity_s* GServer::FindPlayerEntity()
	{
		for (int i = 0; i < level_locals->num_entities; i++)
		{
			gentity_s* ent = &scr_g_entities[i];
			if (ent->r.inuse && ent->classname && ent->classname == scr_const_player)
				return ent;
		}
		return nullptr;
	}

	// The engine picks the corpse slot farthest from the first live player entity, and
	// dereferences that lookup unchecked. Dying while no entity is flagged in use with the
	// "player" classname faults on the read at iw3mp+0xC978D, so fall back to plain slot
	// recycling instead of letting the game read through a null pointer.
	int GServer::GetFreeCorpseSlot()
	{
		if (FindPlayerEntity())
			return G_GetFreeCorpseSlot_h();

		for (int i = 0; i < MAX_CORPSES; i++)
		{
			if (g_corpseInfo[i].entnum == CORPSE_SLOT_FREE)
				return i;
		}
		const int entnum = g_corpseInfo[0].entnum;
		if (level_locals->gentities && entnum >= 0 && entnum < MAX_GENTITIES)
			G_FreeEntity(&level_locals->gentities[entnum]);

		g_corpseInfo[0].entnum = CORPSE_SLOT_FREE;
		return 0;
	}
}
