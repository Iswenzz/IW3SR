#include "Colors.hpp"

namespace IW3SR
{
	static const int* const MyTeam = reinterpret_cast<const int*>(0xCC9F324);
	static const uint32_t* const FirstTeamColor = reinterpret_cast<const uint32_t*>(0xCC9F31C);
	static const uint32_t* const SecondTeamColor = reinterpret_cast<const uint32_t*>(0xCC9F320);

	constexpr int OwnTeam = 2;

	constexpr uint32_t ColorTable[] = {
		0xFF000000, // ^0 black
		0xFF3333FF, // ^1 #ff3333 red, deeper than retail's #ff5c5c
		0xFF00FF00, // ^2 #00ff00 green
		0xFF80FFFF, // ^3 #ffff80 yellow, lighter than retail's #ffff00
		0xFFFF0000, // ^4 #0000ff blue
		0xFFFFFF00, // ^5 #00ffff cyan
		0xFFFF5CFF, // ^6 #ff5cff magenta
		0xFFFFFFFF, // ^7 #ffffff white
		0xFFFFFFFF, // ^8 filler, team colour
		0xFFFFFFFF, // ^9 filler, team colour
		0xFF0000D8, // ^: #d80000 red
		0x7800B83C, // ^; #3cb800 green, deliberately semi transparent
		0xFF1D94F7, // ^< #f7941d orange
		0xFFC57900, // ^= #0079c5 blue
		0xFFC8BEAD, // ^> #adbec8 silver
		0xFF954780, // ^? #804795 purple
		0xFF0059A5, // ^@ #a55900 brown
	};

	void Colors::Lookup(int c, uint32_t* color)
	{
		if (!color)
			return;

		if (c == '8' || c == '9')
		{
			const bool own = *MyTeam == OwnTeam;
			const bool first = (c == '8') == !own;

			*color = first ? *FirstTeamColor : *SecondTeamColor;
			return;
		}
		const int index = c - '0';
		// Clamps an unknown escape to white
		*color = index >= 0 && index < static_cast<int>(std::size(ColorTable)) ? ColorTable[index] : 0xFFFFFFFF;
	}
}
