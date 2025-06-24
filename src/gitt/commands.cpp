#include "commands.h"
#include<filesystem>
#include<iostream>
#include <fstream>
#include "zstr.hpp"
#include <zlib.h>
#include<algorithm>
#include<ranges>
#include <functional>
#include <numeric>
#include <chrono>
#include "constants.h"

#include <openssl/sha.h>

#include <format>
#include "utilities.h"

namespace commands
{
	namespace fs = std::filesystem;
	using namespace std::string_literals;
	using namespace std::string_view_literals;

	namespace utilities
	{

		std::vector<Tree> parse_trees(std::string const& treeHash)
		{
			auto const treeFilePath = constants::objectsDir / treeHash.substr(0, 2) / treeHash.substr(2);
			zstr::ifstream treeStream(treeFilePath.string());
			if (!treeStream) throw std::runtime_error(std::format("Cannot open the file: {}", treeFilePath.string()));
			try
			{
				std::string const blob{ std::istreambuf_iterator<char>(treeStream), std::istreambuf_iterator<char>() };


				std::vector<Tree> entries;

				// 1) Skip over the "tree <size>\0" header
				auto hdr_end = blob.find('\0');
				if (hdr_end == std::string_view::npos) return entries;
				size_t pos = hdr_end + 1;

				// 2) Repeatedly parse <mode> <name>\0<20-byte raw sha>
				while (pos < blob.size()) {
					// find space between mode and name
					auto sp = blob.find(' ', pos);
					if (sp == std::string_view::npos) break;
					std::string mode{ blob.substr(pos, sp - pos) };

					// find terminating NUL after name
					auto nul = blob.find('\0', sp + 1);
					if (auto nul = blob.find('\0'); nul == std::string_view::npos)
					{
						break;
					}
					std::string name{ blob.substr(sp + 1, nul - (sp + 1)) };

					pos = nul + 1;
					if (pos + constants::sha1Size > blob.size()) break;

					// take exactly 20 bytes of raw SHA-1
					auto sha_raw = blob.substr(pos, constants::sha1Size);
					pos += constants::sha1Size;

					entries.push_back({ std::move(mode),
										std::move(name),
										sha_raw
						/*std::format("{:02x}"sv, sha_raw)*/ });
				}

				return entries;

			}

			catch (std::bad_alloc const& e) { std::println(std::cerr, "{}"sv, e.what()); }

			treeStream.close();

			return std::vector<Tree>();
		}

		std::string hexToByteString(const std::string& hex) {
			std::string cleanHex;

			// Remove optional "0x" prefix
			if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) {
				cleanHex = hex.substr(2);
			}
			else {
				cleanHex = hex;
			}

			// Check even length
			if (cleanHex.length() % 2 != 0) {
				throw std::invalid_argument("Hex string must have even length");
			}

			std::string byteStr;
			byteStr.reserve(cleanHex.length() / 2);

			for (size_t i = 0; i < cleanHex.length(); i += 2) {
				std::string byteHex = cleanHex.substr(i, 2);

				// Validate hex digits
				if (!std::isxdigit(byteHex[0]) || !std::isxdigit(byteHex[1])) {
					throw std::invalid_argument("Invalid hex digit");
				}

				char byte = static_cast<char>(std::stoi(byteHex, nullptr, 16));
				byteStr.push_back(byte);
			}

			return byteStr;
		}



	}
	int init()
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

			std::cout << "Initialized git directory in" <<constants::objectsDir.parent_path().string()<< "\n";
			return EXIT_SUCCESS;
		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << e.what() << '\n';
			return EXIT_FAILURE;
		}

	}

	std::expected<std::string, std::string> cat(std::string option, std::string args)
	{
		if (option != "-p") { std::cerr << "Unknown command!\n"; return std::unexpected("Unknown command!\n"); }
		std::filesystem::path const blobPath = constants::objectsDir / std::filesystem::path(args.substr(0, 2)) / args.substr(2);
		try {
			zstr::ifstream input(blobPath.string(), std::ios::binary);
			std::string const objectStr{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
			input.close();

			if (auto const firstNotNull = objectStr.find('\0'); firstNotNull != std::string::npos) {
				std::string const content = objectStr.substr(firstNotNull + 1);
				//std::cout << content << std::flush;
				return content;
			}

			else { return std::unexpected("Not finding the null charachter in the object"); }

		}
		catch (const std::filesystem::filesystem_error& e) {
			//std::println(std::cerr, "{}"sv, e.what());
			return std::unexpected(std::format("Filesystem error {} :", e.what()));
		}
		catch (const std::exception e)
		{
			//std::println(std::cerr, "{}"sv, e.what());
			return std::unexpected(e.what());
		}
		return std::unexpected("Unknown failure in cat");;
	}

	std::expected<std::string, std::string> hash(std::filesystem::path const& path, bool wrtiteThebject, bool print)
	{
		try
		{
			std::ifstream file(path);
			try
			{

				std::string content{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
				file.close();
				std::string const header = "blob " + std::to_string(content.size());
				std::string const finaHashInput = header + '\0' + content;
				return utilities::hash_and_save(finaHashInput, wrtiteThebject);
			}
			catch (const std::bad_alloc& e)
			{
				if (print) std::println(std::cerr, "{}", e.what());
				return std::unexpected{ std::format("{}", e.what()) };
			}

		}
		catch (std::filesystem::filesystem_error const& e)
		{
			if (print) std::println(std::cerr, "Cannot open the file {}", e.what());
			return std::unexpected{ std::format("{}", e.what()) };
		}

	}

	int ls_tree(std::string args, bool namesOnly)
	{
		std::vector<Tree> entries{ utilities::parse_trees(args) };
		auto tree_printer = [](const Tree& t) {return std::format("{} {} {}", t.perm_, t.name_, t.shaHash_); };
		auto projection = namesOnly ? [](const Tree& t) {return t.name_; } : tree_printer;
		for (auto&& x : std::views::transform(entries, projection))
		{
			std::println(std::cout, "{}", x);
		}

		return EXIT_SUCCESS;
	}

	std::expected<std::string, std::string> write_tree_and_get_hash(fs::path const& pathToTree) noexcept
	{
		if (fs::is_empty(pathToTree) && fs::is_directory(pathToTree))
			return {};

		std::vector<fs::directory_entry> vec{};

		for (auto const& de : fs::directory_iterator(pathToTree))
			if (!fs::is_directory(de) || (!fs::is_empty(de) && fs::is_directory(de) && de.path().filename() != ".git"))
				vec.push_back(de);
		std::ranges::sort(vec, {}, [](const fs::directory_entry& de) {return de.path().filename(); });

		struct HashAndEntry { std::string hash{}; fs::directory_entry e{}; };

		auto entriesHashe = std::ranges::transform_view(vec, [](fs::directory_entry const& e)-> HashAndEntry {
			if (fs::is_directory(e)) {
				if (auto hash = write_tree_and_get_hash(e.path()); hash.has_value())
				{
					return HashAndEntry{ utilities::hexToByteString(hash.value()), e };
				}
			}
			else if (auto shaHash = hash(e.path(), true, false); shaHash.has_value())
			{
				return HashAndEntry{ utilities::hexToByteString(shaHash.value()),e };
			}
			else
				// HACK : replace with optioonal return type and make and_then at the caller.
				;
			});

		auto trees = entriesHashe
			| std::views::transform([](HashAndEntry const& x) -> Tree {return Tree(x.e, x.hash); })
			| std::ranges::to<std::vector>();

		auto treeConverter = [](const Tree& t) ->std::string
			{return std::string(t.perm_) + " " + t.name_ + '\0' + t.shaHash_; };

		auto vectorOfContent = trees | std::views::transform(treeConverter);
		std::string const content = std::ranges::fold_left(vectorOfContent, std::string{}, std::plus());
		std::string header = "tree ";

		auto endValue = header + std::to_string(content.size()) + '\0' + content;
		return utilities::hash_and_save(endValue, true);

	}

	int write_tree(std::filesystem::path path)
	{
		if (auto res = write_tree_and_get_hash(path); res.has_value())
		{
			std::cout << res.value(); return EXIT_SUCCESS;
		}
		else
		{
			std::println(std::cout, "{}", res.error()); return EXIT_FAILURE;
		}
	}

	auto commmit(
		std::string const treeHash,
		std::optional<std::string> parentTreeHas,
		std::optional<std::string> msg
	) -> std::expected<std::string, std::string>
	{
		using namespace constants::hardCodedCommitVals;
		using namespace std::chrono;

		auto now = system_clock::now();
		zoned_time zt{ current_zone(), now };  // Local time + offset
		auto epochSeconds = duration_cast<seconds>(now.time_since_epoch()).count();

		std::string parentPart = parentTreeHas ? std::format("parent {}\n"sv, parentTreeHas.value()) : std::string{};
		std::string commiterPart = std::format("committer {} <{}> {} {:%z}\n", committerName, committerMail, epochSeconds, zt);
		std::string authorPart = std::format("author {} <{}> {} {:%z}\n", authorName, authorMail, epochSeconds, zt);
		std::string msgPart = msg ? "\n" + msg.value() : "";

		//TODO: Use commitContentStart constant instead of hard coded tree. 
		std::string content = std::format("tree {}\n{}{}{}{}\n", treeHash, parentPart, authorPart, commiterPart, msgPart);
		std::string endValue = "commit " + std::to_string(content.size()) + '\0' + content;

		auto commitHash = utilities::hash_and_save(endValue, true);
		return commitHash;
	}


	Tree::Tree(std::string_view perm, std::string_view name, std::string_view hash) : perm_(perm), name_(name), shaHash_(hash)
	{
	}

	Tree::Tree(std::filesystem::directory_entry const& de, std::string const& hash)
	{

		std::string const name = de.path().filename().string();
		auto get_mode = [](fs::directory_entry const& e) -> std::string
			{
				if (fs::is_directory(e)) return constants::gitTreeConsts::directory;
				if (fs::is_regular_file(e)) return constants::gitTreeConsts::regularFile;
				else if (fs::is_symlink(e)) return constants::gitTreeConsts::symbolink;
				else return constants::gitTreeConsts::excutableFile;
			};
		name_ = name; perm_ = get_mode(de); shaHash_ = hash;
	}


}
