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


#include <openssl/sha.h>

#include <format>

namespace commands
{
	namespace fs = std::filesystem; 
	using namespace std::string_literals;
	using namespace std::string_view_literals;

	namespace utilities
	{
		std::string sha1_hash(const std::string& content) {
			unsigned char hash[SHA_DIGEST_LENGTH];
			SHA1(reinterpret_cast<const unsigned char*>(content.data()), content.size(), hash);

			std::stringstream ss;
			for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
				ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
			}
			return ss.str();

		}

		std::string zlib_compressed_str(std::string const& input)
		{
			auto compressedSize = compressBound(static_cast<uLong>(input.size()));
			std::string compressed{}; compressed.resize(compressedSize);
			compress(reinterpret_cast<Bytef*>(&compressed[0]), &compressedSize, reinterpret_cast<Bytef const*>(input.data()), static_cast<uLong>(input.size()));
			compressed.resize(compressedSize);
			return compressed;
		}
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

					// turn the 20 bytes into hex
					auto to_hex = [](std::string_view sha)->std::string
						{
							std::ostringstream hexout{};
							hexout << std::hex << std::setfill('0');
							for (auto b : sha)
								hexout << std::setw(2) << (int)static_cast<uint8_t>(b);
							return hexout.str();
						};


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
		if (option != "-p") { std::cerr << "Unknown command!\n"; return EXIT_FAILURE; }
		std::filesystem::path const blobPath = constants::objectsDir / std::filesystem::path(args.substr(0, 2)) / args.substr(2);
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
			std::println(std::cerr, "{}"sv, e.what());
			return EXIT_FAILURE;
		}
		catch (const std::exception e)
		{
			std::println(std::cerr, "{}"sv, e.what());
			return EXIT_FAILURE;
		}
		return 0;
	}

	int hash_command(std::filesystem::path const& path, bool wrtiteThebject, bool print)
	{

		/*try
		{
			std::ifstream file(path);
			try
			{

				std::string content{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
				file.close();
				std::string const header = "blob " + std::to_string(content.size());
				std::string const raw = header + '\0' + content;
				std::string const hashedContent = utilities::sha1_hash(raw);
				if (print)
				{
					std::println(std::cout, "{}", hashedContent);
				}

				if (!wrtiteThebject) return EXIT_SUCCESS;

				std::filesystem::path objectDir = constants::objectsDir / hashedContent.substr(0, 2);
				std::filesystem::create_directories(objectDir);
				const auto filePath = objectDir / hashedContent.substr(2);
				auto compressed = utilities::zlib_compressed_str(raw);
				std::ofstream outputHashStream(filePath);
				if (!outputHashStream) return EXIT_FAILURE;
				outputHashStream << compressed;
				outputHashStream.close();
				return EXIT_SUCCESS;



			}
			catch (const std::bad_alloc& e) { std::println(std::cerr, "{}", e.what()); }

		}
		catch (const std::filesystem::filesystem_error& e)
		{
			std::println(std::cerr, "{}", e.what());
		}

		return EXIT_FAILURE;*/
		auto shaHash = create_hash_and_give_sha(path, wrtiteThebject); 
		if (shaHash.has_value())
		{
			
			if(print) std::println(std::cout, "{}", shaHash.value());
			return EXIT_SUCCESS;
		}
		else
		{
			std::println(std::cerr, "{}", shaHash.error()); return EXIT_FAILURE; 
		}
	}

	int ls_tree_command(std::string args, bool namesOnly)
	{
		std::vector<Tree> entries{ utilities::parse_trees(args) };
		auto tree_printer = [](const Tree& t) {return std::format("{} {} {}", t.perm_, t.name_, t.shaHash_); };
		auto projection = namesOnly ? [](const Tree& t) {return t.name_; } : tree_printer;
		for (auto&& x : std::views::transform(entries, projection))
		{
			std::println(std::cout, "{}", x); 
		}
		
		//for()
		return EXIT_SUCCESS;
	}

	std::expected<std::string,std::string> write_tree_and_get_hash(fs::path const& pathToTree)
	{
		if (fs::is_empty(pathToTree) &&fs::is_directory(pathToTree))
			return {};

		std::vector<fs::directory_entry> vec{};
		
		for (auto const& de : fs::directory_iterator(pathToTree))
			if (!fs::is_directory(de) || (!fs::is_empty(de) && fs::is_directory(de) && de.path().filename() != ".git"))
				vec.push_back(de);
		std::ranges::sort(vec, {}, [](const fs::directory_entry& de) {return de.path().filename(); });
		struct HashAndEntry { std::string hash{}; fs::directory_entry e{}; };
		//auto hash_func = [](fs::directory_entry const& de){ fs::is_directory(de)? }
		auto entriesHashe = std::ranges::transform_view(vec, [](fs::directory_entry const& e)-> HashAndEntry {
			if (fs::is_directory(e)) {
				if (auto hash = write_tree_and_get_hash(e.path()); hash.has_value())
				{
					return HashAndEntry{utilities::hexToByteString(hash.value()), e};
				}
			}
			else if (auto hash = create_hash_and_give_sha(e.path(), true); hash.has_value())
			{
				return HashAndEntry{ utilities::hexToByteString(hash.value()),e};
			}
			});

		auto trees = entriesHashe
			| std::views::transform([](HashAndEntry const&  x) -> Tree {return Tree(x.e, x.hash); })
			|std::ranges::to<std::vector>();
		
		auto treeConverter = [](const Tree& t) ->std::string
			{return std::string(t.perm_) + " " + t.name_ + '\0' + t.shaHash_; };

		auto vectorOfContent = trees | std::views::transform(treeConverter);
		std::string const content = std::ranges::fold_left(vectorOfContent, std::string{}, std::plus<>()); 
		std::string header = "tree ";
		
		auto endValue = header + std::to_string(content.size()) + '\0' + content;
		auto treeHash = utilities::sha1_hash(endValue);
		auto objectDirPath = constants::objectsDir / treeHash.substr(0, 2);
		fs::create_directories(objectDirPath);
		const auto filePath = objectDirPath / treeHash.substr(2);
		auto compressed = utilities::zlib_compressed_str(endValue);
		std::ofstream outputHashStream(filePath);
		if (!outputHashStream) 
			return std::unexpected("EXIT_FAILURE");
		outputHashStream << compressed;
		outputHashStream.close();
		return treeHash;


	}
	
	int write_tree_command()
	{
		if (auto res = write_tree_and_get_hash(fs::path(".")); res.has_value())
		{
			std::cout << res.value(); return EXIT_SUCCESS;
		}
		else 
		{
			std::println(std::cout, "{}", res.error()); return EXIT_FAILURE; 
		}
	}
	

	Tree::Tree(std::string_view perm, std::string_view name, std::string_view hash): perm_(perm), name_(name), shaHash_(hash)
	{
	}

	Tree::Tree(std::filesystem::directory_entry const& de, std::string const& hash)
	{
		//if (de.path().string().empty())
			//int x = 5;
		std::string const name = de.path().filename().string();
		auto get_mode = [](fs::directory_entry const& e) -> std::string
			{
				if (fs::is_directory(e)) return "40000";
				if (fs::is_regular_file(e)) return "100644";
				else if (fs::is_symlink(e)) return "120000";
				else return "100755";
			};
		name_ = name; perm_ = get_mode(de); shaHash_ = hash; 
	}


}

std::expected<std::string, std::string> commands::create_hash_and_give_sha(std::filesystem::path const& path, bool writeTheObject)
{
	try
	{
		std::ifstream file(path);
		try
		{

			std::string content{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
			file.close();
			std::string const header = "blob " + std::to_string(content.size());
			std::string const raw = header + '\0' + content;
			std::string const hashedContent = utilities::sha1_hash(raw);
			/*if (print)
			{
				std::println(std::cout, "{}", hashedContent);
			}*/

			if (!writeTheObject) return hashedContent;

			std::filesystem::path objectDir = constants::objectsDir / hashedContent.substr(0, 2);
			std::filesystem::create_directories(objectDir);
			const auto filePath = objectDir / hashedContent.substr(2);
			auto compressed = utilities::zlib_compressed_str(raw);
			std::ofstream outputHashStream(filePath);
			if (!outputHashStream) return std::unexpected("EXIT_FAILURE");
			outputHashStream << compressed;
			outputHashStream.close();
			return hashedContent;



		}
		catch (const std::bad_alloc& e) { std::println(std::cerr, "{}", e.what()); return std::unexpected(e.what());
		}

	}
	catch (const std::filesystem::filesystem_error& e)
	{
		std::println(std::cerr, "{}", e.what());
		return std::unexpected(e.what());
	}

	return "ERROR";
}
