#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class Autocomplete
	{
	public:
		static void Initialize();
		static void CompleteDvarValue();

	private:
		static std::string Match(const char** strings, int count, const std::string& prefix);
		static void Replace(size_t replaceCount, const std::string& replacement);
	};
}
