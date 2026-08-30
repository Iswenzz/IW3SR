#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	constexpr size_t AesBlockSize = 16;

	class API Aes128
	{
	public:
		explicit Aes128(const uint8_t key[AesBlockSize]);

		bool EncryptCbc(const uint8_t* in, uint8_t* out, size_t length, const uint8_t iv[AesBlockSize]) const;

	private:
		void EncryptBlock(uint8_t block[AesBlockSize]) const;

		uint8_t Round[11][AesBlockSize] = {};
	};
}
