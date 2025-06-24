#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include<sstream>
#include <algorithm>
#include <ranges>
#include <variant>
#include <iterator>
//#include "gitt/clone.h"
#include "gitt/commands.h"
#include "zstr.hpp"
template <class Val, class Err>
int convert_expected(const std::expected<Val, Err>& e, bool print)
{
	if (e)
	{
		if (print) std::cout << e.value() << "\n";
		return EXIT_SUCCESS;
	}
	else
	{
		std::cerr << e.error() << "\n";
		return EXIT_FAILURE;
	}
};

//#define DEBUG
#ifndef DEBUG
int main(int argc, char* argv[]) {
	std::vector<std::string> args(argv, argv + argc);
#else
#include<sstream>
int main() {
	std::string input;
	std::getline(std::cin, input);

	std::istringstream iss(input);
	std::vector<std::string> args((std::istream_iterator<std::string>(iss)),
		std::istream_iterator<std::string>());
	args.insert(args.begin(), "notImportant");
	size_t argc = args.size();

#endif // DEBUG




	// Flush after every std::cout / std::cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	std::cerr << "Logs from your program will appear here!\n";



	if (argc < 2) {
		std::cerr << "No command provided.\n";
		return EXIT_FAILURE;
	}

	std::string command = args[1];



	if (command == "init") {
		return commands::init();
	}
	else if (command == "cat-file") {
		std::string option = args[2];
		std::string arg = args[3];
		auto res = commands::cat(args[2], args[3]);
		return convert_expected(res, true); 

	}
	else if (command == "hash-object")
	{

		std::string arg = args.back();
		bool writeOption = std::ranges::find(args, "-w") != args.end();
		return convert_expected(commands::hash(arg, writeOption, false), true);
	}
	else if (command == "ls-tree")
	{
		std::string arg = args.back();
		bool namesOnly = std::ranges::find(args, "--name-only") != args.end();
		return commands::ls_tree(arg, namesOnly);
	}
	else if (command == "write-tree")
	{
		return commands::write_tree();
	}
	else if (command == "commit-tree")
	{
		std::string treeHash = *std::next(std::ranges::find(args, "commit-tree"));
		std::optional<std::string> parentTreeHash{ std::nullopt };
		auto parentOptionIt = std::ranges::find(args, "-p");
		if (parentOptionIt != args.end())
			parentTreeHash = *std::next(parentOptionIt);

		std::optional<std::string> msg{ std::nullopt };
		if (auto msgOptionIt = std::ranges::find(args, "-m"); msgOptionIt != args.end())
		{
			auto endOfQuotedStringIT = std::ranges::find_if(args, [](std::string_view s) {return s.ends_with('\"'); });
			auto test = std::ranges::join_with_view(std::ranges::subrange(std::next(msgOptionIt), endOfQuotedStringIT), ' ');
			auto reconstructedQuotedMsg = std::ranges::fold_left(test, std::string{}, std::plus());

			msg = reconstructedQuotedMsg.starts_with('\"') and reconstructedQuotedMsg.ends_with('\"') ?
				reconstructedQuotedMsg.substr(1, reconstructedQuotedMsg.size() - 2) :
				reconstructedQuotedMsg;
		}


		return convert_expected(
			commands::commmit(treeHash, parentTreeHash, msg), 
			true
			);
	}
	else if (command == "clone")
	{
		std::string url = args[2];
		std::filesystem::path p = std::filesystem::current_path(); 
		if (argc == 4)
		{
			p = p / args.back();
#ifndef DEBUG
			p = args.back(); 
#endif // !DEBUG

		}
		auto res = commands::clone::clone(url, p);
		return convert_expected(res, true);
	}
	else {
		std::cerr << "Unknown command " << command << '\n';
		return EXIT_FAILURE;
	}

	//return EXIT_SUCCESS;
}
