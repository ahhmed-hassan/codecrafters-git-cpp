#pragma once
#include <sstream>
#include <string>
#include <expected>
#include <filesystem>
namespace commands
{
	namespace utilities
	{
		std::string sha1_hash(const std::string& content);

		//TODO::Make it templated
		std::string zlib_compressed_str(std::string const& input);
		std::string zlib_compressed_str(std::basic_string<unsigned char> const& input);

		std::filesystem::path create_directories_and_get_path_from_hash(
			std::string const& sha, 
			std::filesystem::path const& targetPath= std::filesystem::current_path()
		);
		std::expected<std::string, std::string> hash_and_save(std::string const& toHash, bool save);
		auto binary_sha_to_hex(std::string sha) -> std::string;
	}
		
}
