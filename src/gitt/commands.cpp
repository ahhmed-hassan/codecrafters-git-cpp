#include "commands.h"
#include<filesystem>
#include<iostream>
#include <fstream>
#include "zstr.hpp"
#include <zlib.h>
#include<algorithm>
#include<ranges>
#include <functional>
#include <openssl/sha.h>

#include <format>

namespace commands
{
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
			auto compressedSize = compressBound(input.size());
			std::string compressed{}; compressed.resize(compressedSize);
			compress(reinterpret_cast<Bytef*>(&compressed[0]), &compressedSize, reinterpret_cast<Bytef const*>(input.data()), input.size());
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
										to_hex(sha_raw)
						/*std::format("{:02x}"sv, sha_raw)*/ });
				}

				return entries;

			}

			catch (std::bad_alloc const& e) { std::println(std::cerr, "{}"sv, e.what()); }

			treeStream.close();

			return std::vector<Tree>();
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

	int hash_command(std::filesystem::path const& path, bool wrtiteThebject)
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
				std::println(std::cout, "{}", hashedContent);

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

		//return 0;
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
	

}
