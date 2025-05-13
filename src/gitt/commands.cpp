#include "commands.h"
#include<filesystem>
#include<iostream>
#include <fstream>
#include "zstr.hpp"
#include <zlib.h>
#include <openssl/sha.h>

#include <format>
namespace utilties
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
}
namespace commands
{
	using namespace std::string_literals; 
	using namespace std::string_view_literals;
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
				std::string const hashedContent = utilties::sha1_hash(raw); 
				std::println(std::cout, "{}", hashedContent);

				if (!wrtiteThebject) return EXIT_SUCCESS; 	

				std::filesystem::path objectDir = constants::objectsDir / hashedContent.substr(0, 2); 
				std::filesystem::create_directories(objectDir); 
				const auto filePath = objectDir / hashedContent.substr(2); 
				auto compressed = utilties::zlib_compressed_str(raw); 
				std::ofstream outputHashStream(filePath); 
				if (!outputHashStream) return EXIT_FAILURE; 
				outputHashStream << compressed; 
				outputHashStream.close(); 
				return EXIT_SUCCESS; 
				
				

			}
			catch (const std::bad_alloc& e) { std::println(std::cerr,"{}", e.what()); }
			
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			std::println(std::cerr, "{}", e.what());
		}
		
		//return 0;
	}

}
