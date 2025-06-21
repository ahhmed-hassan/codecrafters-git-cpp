#include "utilities.h"
#include<filesystem>
#include<iostream>
#include <fstream>
#include "zstr.hpp"
#include <zlib.h>
#include<algorithm>
#include<ranges>
#include <functional>
#include <numeric>
#include <chrono>
#include "constants.h"

#include <openssl/sha.h>

#include <format>
namespace commands
{
	namespace fs = std::filesystem;
	namespace utilities
	{
		std::string sha1_hash(const std::string& content) {
			unsigned char hash[SHA_DIGEST_LENGTH];
			SHA1(reinterpret_cast<const unsigned char*>(content.data()), content.size(), hash);

			std::stringstream ss;
			for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
				ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
			}
			return ss.str();

		}

		std::string zlib_compressed_str(std::string const& input)
		{
			auto compressedSize = compressBound(static_cast<uLong>(input.size()));
			std::string compressed{}; compressed.resize(compressedSize);
			compress(reinterpret_cast<Bytef*>(&compressed[0]), &compressedSize, reinterpret_cast<Bytef const*>(input.data()), static_cast<uLong>(input.size()));
			compressed.resize(compressedSize);
			return compressed;
		}

		std::string zlib_compressed_str(std::basic_string<unsigned char> const& input)
		{
			auto compressedSize = compressBound(static_cast<uLong>(input.size()));
			std::string compressed{}; compressed.resize(compressedSize);
			compress(reinterpret_cast<Bytef*>(&compressed[0]), &compressedSize, reinterpret_cast<Bytef const*>(input.data()), static_cast<uLong>(input.size()));
			compressed.resize(compressedSize);
			return compressed;
		}

		std::filesystem::path create_directories_and_get_path_from_hash(
			std::string const& objectHash, 
			std::filesystem::path const& targetBeginPath
			)
		{
			auto objectDirPath = targetBeginPath / constants::objectsDir / objectHash.substr(0, 2);
			fs::create_directories(objectDirPath);
			const auto filePath = objectDirPath / objectHash.substr(2);
			return filePath;
		}

		std::expected<std::string, std::string> hash_and_save(std::string const& toHash, bool save)
		{
			auto objectHash = utilities::sha1_hash(toHash);
			auto objectDirPath = constants::objectsDir / objectHash.substr(0, 2);
			fs::create_directories(objectDirPath);
			const auto filePath = objectDirPath / objectHash.substr(2);
			auto compressed = utilities::zlib_compressed_str(toHash);

			if (!save)
				return objectHash;

			std::ofstream outputHashStream(filePath);
			if (!outputHashStream)
				return std::unexpected(std::format("Cannot open the file: \t {}", filePath.string()));
			outputHashStream << compressed;
			outputHashStream.close();
			return objectHash;

		}
		auto to_hex(std::string_view sha) -> std::string
		{

			std::ostringstream hexout{};
			hexout << std::hex << std::setfill('0');
			for (auto b : sha)
				hexout << std::setw(2) << (int)static_cast<uint8_t>(b);
			return hexout.str();

		}
	}
}