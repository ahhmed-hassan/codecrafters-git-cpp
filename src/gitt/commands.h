#pragma once
#include<string>
#include<string_view>
#include <filesystem>
#include <vector>
#include <map>
#include <expected>
namespace commands
{
	namespace constants
	{
		std::filesystem::path const gitDir = ".git" ;
		std::filesystem::path const objectsDir = gitDir / "objects";
		std::filesystem::path const refsDir = gitDir / "refs";
		std::filesystem::path const head = gitDir / "HEAD"; 
		size_t const sha1Size = 20ul;
		namespace gitConsts 
		{
			std::string const regularFile{ "10644" }; 
			std::string const excutableFile{ "100755" }; 
			std::string const symbolink{ "12000" }; 
			std::string const directory{ "40000" }; 
			 
			enum struct Permession
			{
				regularFile, executableFile, symbollink, directory
			};
			std::map < std::string, Permession > const strToPermession
			{ 
				{regularFile, Permession::regularFile},{excutableFile, Permession::executableFile},{directory, Permession::directory},{symbolink, Permession::symbollink} 
			};
		}
	}
	struct Tree
	{
		std::string perm_{}; 
		std::string name_{}; 
		std::string shaHash_{}; 
		Tree() = default; 
		Tree(std::string_view perm, std::string_view name, std::string_view hash);
		Tree(std::filesystem::directory_entry const& de, std::string const& hash); 
	};
	int init_command();
	int cat_command(std::string option, std::string args);
	int hash_command(std::filesystem::path const& path, bool wrtiteThebject, bool print = false);
	
	std::expected<std::string, std::string> create_hash_and_give_sha(std::filesystem::path const& path, bool writeTheObject);
	
	int ls_tree_command(std::string args, bool namesOnly); 
	int write_tree_command();
	namespace utilities
	{
		std::vector<Tree> parse_trees(std::string const& treeHash = "3d59ede50c909c5d64b8cdb3cbe31992be17e447");
	}
}