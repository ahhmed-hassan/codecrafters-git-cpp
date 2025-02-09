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
		std::filesystem::path const refsDir = gitDir / "/refs";
		std::filesystem::path const head = gitDir / "/HEAD"; 
	}
	int init_command(std::string command);
	int cat_command(std::string command);
}