#include "commands.h"
#include<filesystem>
#include<iostream>
#include <fstream>
#include "zstr.hpp"
#include <format>
namespace commands
{
	int init_command()
	{

		try {
			//std::filesystem::create_directory(constants::gitDir);
			std::filesystem::create_directories(constants::objectsDir);
			std::filesystem::create_directories(constants::refsDir);

			std::ofstream headFile(constants::head);
			if (headFile.is_open()) {
				headFile << "ref: refs/heads/main\n";
				headFile.close();
			}
			else {
				std::cerr << std::format("Failed to create {} file\n", constants::head.string());
				return EXIT_FAILURE;
			}

			std::cout << "Initialized git directory\n";
			return EXIT_SUCCESS;
		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << e.what() << '\n';
			return EXIT_FAILURE;
		}

	}

	int cat_command(std::string option, std::string args)
	{
		if (option != "-p") { std::cerr << "Unknown command!\n"; return EXIT_FAILURE;  }
		std::filesystem::path const blobPath =constants::objectsDir/ std::filesystem::path(args.substr(0, 2)) / args.substr(2);
		std::filesystem::create_directories(blobPath.parent_path());
		try {
			zstr::ifstream input(blobPath.string(), std::ios::binary);
			std::string const objectStr{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
			input.close();

			if (auto const firstNotNull = objectStr.find('\0'); firstNotNull != std::string::npos) {
				std::string const content = objectStr.substr(firstNotNull + 1);
				std::cout << content << std::flush;
				return EXIT_SUCCESS;
			}

			else { return EXIT_FAILURE; }

		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << e.what() << "\n";
			return EXIT_FAILURE;
		}
		return 0;
	}
}
