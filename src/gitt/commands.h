#pragma once
#include <filesystem>
#include <map>
#include<string>
#include<string_view>
#include <vector>


#include <expected>
namespace commands
{

	struct Tree
	{
		std::string perm_{};
		std::string name_{};
		std::string shaHash_{};
		Tree() = default;
		Tree(std::string_view perm, std::string_view name, std::string_view hash);
		Tree(std::filesystem::directory_entry const& de, std::string const& hash);
	};
	int init(std::filesystem::path const& beginPath);
	std::expected<std::string, std::string> cat(std::string option, std::string args);
	/*
	* Realizing of git hash-object -w for some given path.
	*/
	std::expected<std::string, std::string> hash(std::filesystem::path const& path, bool wrtiteThebject, bool print);


	int ls_tree(std::string args, bool namesOnly);
	std::expected<std::string, std::string> write_tree(std::filesystem::path path = ".");
	auto commmit(std::string const treeHash, std::optional<std::string> parentTreeHash, std::optional<std::string> message = std::nullopt)
		-> std::expected<std::string, std::string>;
	namespace utilities
	{
		/*
		*Hashes the given content
		* @parameter toHash: The uncompressed string to hash
		* @paramater save: If true: The compressed content would be stored in the objects database.
		* @return the sha hash of the content
		*/
		std::expected<std::string, std::string> hash_and_save(std::string const& toHash, bool save = true);
		[[nodiscard]] std::vector<Tree> parse_trees(std::string const& treeHash);
	}

	namespace clone
	{
		std::expected<std::string, std::string> clone(
			std::string const url,
			std::filesystem::path const& beginPath = std::filesystem::current_path());
	}
	
}