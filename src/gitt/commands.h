#pragma once
#include<string>
#include<string_view>
#include <filesystem>
#include <vector>
#include <map>


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
	int init();
	int cat(std::string option, std::string args);
	std::expected<std::string, std::string> hash(std::filesystem::path const& path, bool wrtiteThebject, bool print);
	
	
	int ls_tree(std::string args, bool namesOnly); 
	int write_tree();
	int commmit(std::string const treeHash, std::optional<std::string> parentTreeHash, std::optional<std::string> message = std::nullopt);
	namespace utilities
	{
		std::expected<std::string, std::string> hash_and_save(std::string const& toHash, bool save=true);
		[[nodiscard]] std::vector<Tree> parse_trees(std::string const& treeHash);
	}
}