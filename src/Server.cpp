#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include<sstream>
#include <algorithm>
#include <ranges>
#include <variant>
#include "gitt/clone.h"
//#include <view>
#include "gitt/commands.h"
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
		std::cerr << e.value() << "\n";
		return EXIT_FAILURE;
	}
};

//#define DEBUG
#ifndef DEBUG
int main(int argc, char* argv[]) {
	std::vector<std::string> args(argv, argv + argc);
	auto url = "https://github.com/git/git";
	url = "https://github.com/git/git-reference";
	//std::cout << h<<"\n";https
	auto g = clone::parse_info_refs(clone::get_info_refs());
	using namespace std::string_view_literals;
	auto head = clone::get_head(url);
	//clone::fetch_packfile(url, head);
	//std::println(std::cout, "{}"sv, head.ref.object_id); 
	//auto shaHead = clone::get_head_sha(url);
	
	//std::cout<< "\n\n" <<clone::get_head_sha() << "\n";
	//std::cout << clone::fetch_packfile(url, head) <<"\n\n";
	//std::println(std::cout, "{}", clone::extract_packFile(""));
	try {
		std::string packfile = clone::fetch_packfile(url, head);
		std::cout << "Received packfile: " << packfile.size() << " bytes\n";
		std::cout << "First 20 bytes: " << packfile.substr(0, 20) << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
	}
	return 0;

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
		return commands::cat(args[2], args[3]);
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


		return commands::commmit(treeHash, parentTreeHash, msg);
	}
	else {
		std::cerr << "Unknown command " << command << '\n';
		return EXIT_FAILURE;
	}

	//return EXIT_SUCCESS;
}
