#pragma once
#include <filesystem>
#include <string>
#include <map>
namespace commands
{
	namespace constants
	{
		std::filesystem::path const gitDir = ".git";
		std::filesystem::path const objectsDir = gitDir / "objects";
		std::filesystem::path const refsDir = gitDir / "refs";
		std::filesystem::path const head = gitDir / "HEAD";
		namespace hardCodedCommitVals
		{
			std::string const committerName{ "FooBar" };
			std::string const committerMail{ "FooBar@gmail.com" };
			std::string const authorName{ committerName };
			std::string const authorMail{ committerMail };
		}
		size_t const sha1Size = 20ul;
		namespace gitTreeConsts
		{
			std::string const regularFile{ "100644" };
			std::string const excutableFile{ "100755" };
			std::string const symbolink{ "120000" };
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
}