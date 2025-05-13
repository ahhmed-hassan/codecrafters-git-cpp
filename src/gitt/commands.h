#pragma once
#include<string>
#include<string_view>
#include <filesystem>
namespace commands
{
	namespace constants
	{
		std::filesystem::path const gitDir = ".git" ;
		std::filesystem::path const objectsDir = gitDir / "objects";
		std::filesystem::path const refsDir = gitDir / "refs";
		std::filesystem::path const head = gitDir / "HEAD"; 

		namespace gitConsts 
		{
			std::string const regularFile{ "10644" }; 
			std::string const excutableFile{ "100755" }; 
			std::string const symbolink{ "12000" }; 
			std::string const directory{ "40000" }; 
		}
	}
	int init_command();
	int cat_command(std::string option, std::string args);
	int hash_command(std::filesystem::path const& path, bool wrtiteThebject);
}