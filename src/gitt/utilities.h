#pragma once
#include <sstream>
#include <string>
#include <expected>
namespace commands
{
	namespace utilities
	{
	std::string sha1_hash(const std::string& content);
		

	std::string zlib_compressed_str(std::string const& input);
	std::string zlib_compressed_str(std::basic_string<unsigned char> const& input);

	std::expected<std::string, std::string> hash_and_save(std::string const& toHash, bool save);
	}
}
