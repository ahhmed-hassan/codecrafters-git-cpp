#include <string>
#include <asio.hpp>
#include <cpr/cpr.h>
#include <iostream>
#include "constants.h"
#include "utilities.h"
#include "clone.h"
//#include "commands.h"

namespace clone
{
	HeadRef get_head(std::string const& url)
	{
		auto response = internal::get_refs_info(url);
		auto refs = internal::parse_refs_info(response);
		auto headRef = std::get<HeadRef>(*refs.begin());
		return headRef;
	}


	template<typename CharT = char>
	std::basic_string<CharT> fetch_packfile(const std::string& url, HeadRef head) {
		/*std::string upload_pack_url = std::format("{}/git-upload-pack", url);
		std::string body = build_negotiation_body(head);
		std::println(std::cout, "Body\n-------------\n{}", body);
		cpr::Response r = cpr::Post(
			cpr::Url{ upload_pack_url },
			cpr::Body{ body },
			cpr::Header{
				{"Content-Type", "application/x-git-upload-pack-request"},
				{"User-Agent", "my-git-client/1.0"}
			}

		);
		std::println(std::cout, "Content\n------\n {}", r.text);

		if (r.status_code != 200) {
			throw std::runtime_error("Failed to fetch packfile");
		}
		return r.text;
	  */

		std::string upload_pack_url = url + "/git-upload-pack";

		// 2. Minimal body without capabilities
		std::string body = internal::build_negotiation_body(head);
		body = std::format("0032want {}\n00000009done\n", head.ref.object_id);

		// 3. Simplified headers
		cpr::Response r = cpr::Post(
			cpr::Url{ upload_pack_url },
			cpr::Body{ body },
			cpr::Header{
				{"Content-Type", "application/x-git-upload-pack-request"}
				// No "Accept" header!
			}
		);

		if (r.status_code != 200 || r.text.empty()) {
			throw std::runtime_error("Failed to fetch packfile");
		}
		return std::basic_string<CharT>{ r.text.begin(), r.text.end() };
		
	}

	template <class CharT = char>
	PackHeader extract_packHeader(std::basic_string<CharT> const& packData)
	{
		if (packData.size() < 12) throw std::runtime_error("Invlid packfile size");

#pragma region HeaderInitialization
		PackHeader header{};
		using namespace commands::constants::clone;
		std::memcpy(&header, packData.data() + sizeof(startOfHeader.data()), sizeof(PackHeader));
		header.version = ntohl(header.version);
		header.objectCount = ntohl(header.objectCount);
#pragma endregion


#pragma region Headerverification
		if(std::memcmp(header.magic, magicPack.data(), sizeof(magicPack.data()) != 0))
			throw std::runtime_error("Invalid packfile singature");

		if (std::ranges::find(acceptableVersions, header.version) == std::end(acceptableVersions))
			throw std::runtime_error("Unsupported packfile version");
#pragma endregion 


		return header;
	}

	void process_packfile(packstring const& packData)
	{
		PackHeader packHeader = extract_packHeader(packData); 

		size_t objectOffset = commands::constants::clone::objectsBeginPos; 
		for (uint32_t i{}; i < packHeader.objectCount; i++)
		{
			internal::ObjectHeader objHeader = internal::get_object_header_beginning_at(packData, objectOffset); 

			//Move to the actual compressed Data for the header
			objectOffset += objHeader.headerBytes; 

			auto dataCompressed = packData.substr(
				objectOffset,
				packData.size() - objectOffset - 20
			);
			
			internal::process_git_object(objHeader.is_deltified(), dataCompressed);

			//Move To Next Object
			objectOffset += dataCompressed.size(); 
		}
	}

	
	namespace delta
	{
		void resolveDeltaRefs(
			std::unordered_map<std::string, GitObject>& objectMap,
			std::list<GitObject>& deltaRefs){}
	}

#pragma region GitPackParser
	GitPackParser::GitPackParser(const std::string& inputPack) :
		_input(inputPack),
		_dataSource(std::make_shared<internal::StringDataSource<>>(inputPack))
	{
		init_map();
	}

#ifdef ZLIB_LOWLEVEL
	auto GitPackParser::parse_bin_pack() -> std::expected<std::unordered_map<std::string, GitObject>, std::string>
	{
		Bytef* rawData = reinterpret_cast<Bytef*>(_input.data());
		auto numObjects = parse_header().objectCount;
		// The header parses 20 bytes
		rawData += 20;
		z_stream stream;
		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.opaque = Z_NULL;
		// TODO: Is it OK that I'm setting avail_in and next_in here?
		stream.avail_in = _input.size() - 20;
		stream.next_in = rawData;
		if (inflateInit(&stream) != Z_OK) {
			return std::unexpected{"Failed to initalize zlib decompression"};
		}

		int zlibReturn = Z_OK;
		int bytesRemaining = _input.size() - 20;
		std::unordered_map<std::string, GitObject> objectMap;
		std::list<GitObject> deltaRefs;
		for (size_t i = 0; i < numObjects; ++i) {
			inflateReset(&stream);
			// auto [nextObject, bytesParsed] = parseNextObject(rawData, &stream, zlibReturn);
			// results.push_back(nextObject);
			// rawData += bytesParsed;
			auto [uncompressedSize, bytesParsed, objectType] = parse_objectHeader();
			// std::cerr << "HEADER BYTES PARSED: " << bytesParsed << std::endl;
			GitObject result{ "", objectType, "", "" };
			if (result.type == ObjectType::REF_DELTA) {
				// This is actually the base hash, but we'll fix that later.
				result.hash = parse_hash_20();
				rawData += 20;
				stream.avail_in -= 20;
			}
			bytesRemaining -= bytesParsed;
			rawData += bytesParsed;
			stream.avail_in -= bytesParsed;
			auto prevAvailIn = stream.avail_in;
			stream.next_in = rawData;
			stream.avail_out = uncompressedSize;
			Bytef* buffer = new Bytef[uncompressedSize];
			stream.next_out = buffer;
			// std::cerr << "BEFORE INFLATE: " << stream.avail_in << " " << stream.avail_out << std::endl;
			zlibReturn = inflate(&stream, Z_NO_FLUSH);
			// std::cerr << "AFTER INFLATE: " << stream.avail_in << " " << stream.avail_out << " " << zlibReturn << std::endl;
			if (zlibReturn != Z_STREAM_ERROR) {
				size_t bytesDecompressed = uncompressedSize - stream.avail_out;
				result.uncompressedData.insert(result.uncompressedData.end(), buffer, buffer + bytesDecompressed);
				if (bytesDecompressed != uncompressedSize) {
					std::cerr << "EXPECTED TO DECOMPRESS: " << uncompressedSize << " but only got: " << bytesDecompressed << " " << result.uncompressedData << " " << prevAvailIn << " " << stream.avail_in << " " << stream.avail_out << " " << sizeof(buffer) << std::endl;
				}
				size_t compressedBytesProcessed = prevAvailIn - stream.avail_in;
				result.compressedData = _dataSource->advanceN(compressedBytesProcessed);
				rawData += compressedBytesProcessed;
			}
			// std::cerr << result.uncompressedData << std::endl;
			if (result.type == ObjectType::BLOB || result.type == ObjectType::TREE || result.type == ObjectType::COMMIT) {
				// Adds object header
				auto typeAsStr = result.get_type_for_non_deltiifed(); 
				auto objectToCompress = typeAsStr + std::to_string(result.uncompressedData.size()) + '\0' + result.uncompressedData;
				auto hashResult = commands::utilities::hash_and_save(objectToCompress, false);
				if (!hashResult.has_value()) return std::unexpected(hashResult.error());
				result.hash = hashResult.value(); 
				std::string compressionError;
				// TODO: Check error here?
				result.compressedData = commands::utilities::zlib_compressed_str(objectToCompress);
				objectMap[result.hash] = result;
			}
			else if (result.type == ObjectType::REF_DELTA) {
				deltaRefs.push_back(result);
			}
			else {
				std::cerr << "UNKNOWN OBJECT TYPE...skipping!" << std::endl;
			}
			delete[] buffer;
			if (zlibReturn == Z_STREAM_ERROR) {
				return std::unexpected("Failure to decompress object: " + std::to_string(zlibReturn));
				}
		}
		inflateEnd(&stream);
		if (zlibReturn != Z_STREAM_END) {
			return std::unexpected("Zlib stream did not finish at end: " + std::to_string(zlibReturn));
		}
		delta::resolveDeltaRefs(objectMap, deltaRefs);
		return objectMap;
	}
#else 
	auto GitPackParser::parse_bin_pack() -> std::expected<std::unordered_map<std::string, GitObject>, std::string>
	{
		return std::expected<std::unordered_map<std::string, GitObject>, std::string>();
	}

#endif // DEBUG


	PackHeader GitPackParser::parse_header()
	{
		auto res = extract_packHeader(this->_input);
		this->_dataSource->advanceN(commands::constants::clone::objectsBeginPos);
		return res;
	}

	std::string GitPackParser::parse_hash_20()
	{
		auto toHexChar = [](unsigned char in) -> std::string {
			if (in > 15) throw std::out_of_range("Value out of hex range");
			return std::format("{:x}", in); };
		// Parse 20 bytes, turning each one into 2 hexademical characters
		std::ostringstream oss;
		for (size_t i = 0; i < 20; ++i) {
			char next = _dataSource->advance();
			char second = next & 0x0F;
			char first = (next >> 4) & 0x0F;
			oss << toHexChar(first) << toHexChar(second);
		}
		return oss.str();
	}

	PackObjectHeader GitPackParser::parse_objectHeader()
	{
		char currentByte = _dataSource->advance();
		size_t bytesParsed = 1;
		size_t currentSize = 0;
		constexpr char msbMask = 0x80;
		constexpr char typeMask = 0x70;
		constexpr char last4Mask = 0x0F;
		constexpr char last7Mask = 0x7F;
		char typeChar = (typeMask & currentByte) >> 4;
		auto objectType = static_cast<ObjectType>(typeChar);

		//bool keepParsing = (msbMask & currentByte) != 0;
		auto keepParsing = [msbMask](char currentByte) {return (msbMask & currentByte) != 0; };
		currentSize += static_cast<unsigned char>(last4Mask & currentByte);
		size_t bitsInSize = 4;
		while (keepParsing(currentByte)) {
			currentByte = _dataSource->advance();
			++bytesParsed;
			//keepParsing = (msbMask & currentByte) != 0;
			auto next = static_cast<unsigned char>(last7Mask & currentByte);
			currentSize += (next << bitsInSize);
			bitsInSize += 7;
		}
		return PackObjectHeader{
			.decompressedSize = currentSize,
			.bytesParsed = bytesParsed,
			.type = objectType };
	}


	void GitPackParser::init_map()
	{
	}

}
#pragma endregion
