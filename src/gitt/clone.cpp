#include <string>
#include <asio.hpp>
#include <cpr/cpr.h>
#include <iostream>
#include "constants.h"
#include "utilities.h"
#include "clone.h"
#include "commands.h"

namespace commands
{
	namespace clone
	{
		template <typename CharT = char>
		using DatasourcePtr = std::shared_ptr<internal::DataSource<CharT>>;

		HeadRef get_head(std::string const& url)
		{
			auto response = internal::get_refs_info(url);
			auto refs = internal::parse_refs_info(response);
			auto headRef = std::get<HeadRef>(*refs.begin());
			return headRef;
		}


		template<typename CharT = char>
		std::basic_string<CharT> fetch_packfile(
			const std::string& url,
			HeadRef head) {
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
			using namespace  constants::clone;
			std::memcpy(&header, packData.data() + sizeof(startOfHeader.data()), sizeof(PackHeader));
			header.version = ntohl(header.version);
			header.objectCount = ntohl(header.objectCount);
#pragma endregion


#pragma region Headerverification
			if (std::memcmp(header.magic, magicPack.data(), sizeof(magicPack.data()) != 0))
				throw std::runtime_error("Invalid packfile singature");

			if (std::ranges::find(acceptableVersions, header.version) == std::end(acceptableVersions))
				throw std::runtime_error("Unsupported packfile version");
#pragma endregion 


			return header;
		}

		void process_packfile(packstring const& packData)
		{
			PackHeader packHeader = extract_packHeader(packData);

			size_t objectOffset =  constants::clone::objectsBeginPos;
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

		namespace internal
		{
			size_t parse_variable_length_size(
				DatasourcePtr<> const& src
			)
			{

				const char last7Mask = 0x7F;
				const char msbMask = 0x80;
				auto currentByte = src->advance();
				auto keepParsingg = [msbMask](char currentByte) {
					return (msbMask & currentByte) != 0;  };
				size_t currentSize = 0;
				currentSize += static_cast<unsigned char>(last7Mask & currentByte);
				size_t bitsInSize = 7;
				while (keepParsingg(currentByte)) {
					currentByte = src->advance();
					auto next = static_cast<unsigned char>(last7Mask & currentByte);
					currentSize += (next << bitsInSize);
					bitsInSize += 7;
				}
				return currentSize;

			}

			std::string binary_sha_to_hex(
				DatasourcePtr<> const& ds
			)
			{
				auto toHexChar = [](unsigned char in) -> std::string {
					if (in > 15) throw std::out_of_range("Value out of hex range");
					return std::format("{:x}", in); };
				std::ostringstream oss;
				for (size_t i = 0; i < constants::sha1Size; ++i) {
					char next = ds->advance();
					char second = next & 0x0F;
					char first = (next >> 4) & 0x0F;
					oss << toHexChar(first) << toHexChar(second);
				}
				return oss.str();
				
				/*if (std::ranges::any_of(input, [](auto x) {return x == EOF; }))
					throw std::out_of_range("Too small dataSource!");
					*/
				/*auto input = ds->advanceN(constants::sha1Size);
				return utilities::binary_sha_to_hex(input);*/
			}
		
			DeltarefObject from_gitobject(
				GitObject const& obj
			) {
				return DeltarefObject{ ._referenedHash = obj.hash, ._instructions = obj.uncompressedData };
			}
		}

		namespace delta
		{
			/**
			*Delta Ref Isntruction ahs the following structure :
			* [base_size] [target_size] [instruction1] [instruction2] ... [instructionN]
			* instructon_i = [copy] | [insert]
			* insert = [0[7bits number of bytes to be inserted]] [x bytes of data]
			*
			* copy = [1[4-6 3 bits of representing the existing size chunks][0-3 4 bits representing the exisitng offset]] [offset bytes] [size bytes]
			* Examples of the copy [1 101	1001] [first offset chunk for the first one on righ] [Second offset chunk should be shifted with 8*3 = 24] [first size Chunk for most right size] [second size byte for the next existing size bit]
			*
			*
			**/
			DeltaRefInstruction parse_next_deltarefinstruction(
				DatasourcePtr<> const& src
			)
			{
				char command = src->advance();
				const char msbMask = 0x80;
				if (bool isCopy = (msbMask & command) != 0; isCopy) {

					const char offsetMask = 0x0F;
					/*
					* As we try to parse variable length we should shift by the rank of this but * 8
					* So for example for 0x20 = 010 0000 , we know the size chunk for it would be shifted by 8 and added to whatever left or has alredy been parsed.
					*
					*/
					std::array<std::pair<uint8_t, uint32_t>, 4> const sizeMap{
						std::make_pair(0x10,0),
						{ 0x20, 8},
						{ 0x40, 16 },
					};

					std::array<std::pair<uint8_t, uint32_t>, 4> const offsetMap{
						std::make_pair(0x01,0),
						{ 0x02, 8},
						{ 0x04, 16 },
						{0x08, 24},
					};

					auto getShiftsFromMapAndCommand = [command](std::array<std::pair<uint8_t, uint32_t>, 4> const map)
						-> std::vector<uint32_t> {
						return map | std::views::filter([command](auto pair) {
							return (command & pair.first) != 0; })
							| std::views::values
							| std::ranges::to<std::vector<uint32_t>>();
						};
					auto sizeShifts = getShiftsFromMapAndCommand(sizeMap);


					// bytes in offset (bits 0-3)
					auto offsetShifts = getShiftsFromMapAndCommand(offsetMap);

					size_t copySize = 0;
					size_t copyOffset = 0;
					bool multiplyNotShift(false);
					for (auto shift : offsetShifts) {
						auto next = static_cast<unsigned char>(src->advance());
						//TODO: Replace with copyOffset += static_cast<size_t>(next) * std::pow(2, shift);
						copyOffset += (static_cast<size_t>(next) << shift);
					}
					for (auto shift : sizeShifts) {
						auto next = static_cast<unsigned char>(src->advance());
						copySize += (static_cast<size_t>(next) << shift);
					}
					return CopyInstruction{ .offset = copyOffset, .size = copySize };
				}
				else {
					// bytes to insert (bites 0-6)
					const char last7Mask = 0x7F;
					size_t numInsertBytes = static_cast<size_t>(command & last7Mask);
					// Parse the bytes to insert
					std::string newData = "";
					for (size_t i = 0; i < numInsertBytes; ++i) {
						newData.push_back(src->advance());
					}
					return InsertInstruction{
						.numBytesToInsert = numInsertBytes ,
						 .dataToInsert = std::move(newData),
					};
				}
			}


			std::string build_deltadata_from_instructions(const std::string& instructions, const std::string& referencedData) {
				auto ds = std::make_shared<internal::StringDataSource<>>(instructions);
				//Parse base size
				auto s1 = internal::parse_variable_length_size(ds);
				//Parsing the target size
				internal::parse_variable_length_size(ds);
				if (s1 != referencedData.size()) {
					std::cerr << "Parsed size does not match referenced size! " << s1 << " " << referencedData.size() << " " << referencedData << std::endl;
				}
				auto applyDelta = overload(
					[&referencedData = std::as_const(referencedData)](CopyInstruction const& copy) {return copy.apply_delta(referencedData); },
					[](InsertInstruction const& insert) {return insert.dataToInsert; }
				);
				std::ostringstream oss;
				while (!ds->isAtEnd()) {
					auto instr = parse_next_deltarefinstruction(ds);
					oss << std::visit(applyDelta, instr);
				}
				return oss.str();
			}

			GitObject build_object_from_reference(
				const DeltarefObject& deltaRef,
				const GitObject& referencedObject) {
				GitObject finalObject;
				finalObject.type = referencedObject.type;
				finalObject.uncompressedData = build_deltadata_from_instructions(deltaRef._instructions, referencedObject.uncompressedData);

				auto dataToCompress = finalObject.compress_input(); 

				finalObject.compressedData = utilities::zlib_compressed_str(dataToCompress);
				auto hash = utilities::hash_and_save(dataToCompress, false);
				if (!hash) throw std::runtime_error(hash.error());
				finalObject.hash = hash.value();
				return finalObject;
			}
			/**
			* @brief resolves the passed refs and add them to the map.
			* @postcondition : Map size = map size + queue size
			*
			* @param resolvedObjectMap the map that has all already resolved Objects
			* @param deltaRefs :Refs to be resolved
			*
			* Alternatively this can be thought of as topolgical sorting where each delta is a node,
			* where deltas can only reference nodes that has no edges where edge from n to m
			* means that n is delta and m is base or delta, where deltas have no outgoing edges
			**/
			[[nodiscard]] auto resolve_delta_refs(
				const std::unordered_map<std::string, GitObject>& baseObjectsMap,
				std::queue<DeltarefObject>& deltaRefs
			) -> std::unordered_map<std::string, GitObject>
			{
				std::unordered_map<std::string, size_t> sizeOFdeltasWhenFailed{};
				std::unordered_map<std::string, GitObject>allResolvedObjects{ baseObjectsMap };
				while (!deltaRefs.empty()) {
					auto nextRef = deltaRefs.front();
					deltaRefs.pop();
					auto referencedHash = nextRef._referenedHash;
					if (allResolvedObjects.contains(referencedHash)) {
						auto referencedObject = allResolvedObjects.at(referencedHash);
						auto newObject = build_object_from_reference(nextRef, referencedObject);
						auto [_ , emplaced] = allResolvedObjects.try_emplace(newObject.hash,newObject);
						if (!emplaced) std::cerr << std::format("{} was already resolved!", newObject.hash);
					}
					else {

						deltaRefs.push(nextRef);
						// Can't dereference this hash yet.
						if (
							sizeOFdeltasWhenFailed.contains(referencedHash) &&
							//It is okay to fail again and thus we can expect to fine the referencedHash multple times, but the question is are we making progrss? 
							// i.e, Are we resolving some deltas since last time we have seen this object? 
							sizeOFdeltasWhenFailed.at(referencedHash) <= deltaRefs.size()
							) {
							std::cerr << "Could not find reference for hash: " << referencedHash << " and it looks like an infinite loop!" << std::endl;
							break;
						}
						else { //This is the first time we fail.. 
							sizeOFdeltasWhenFailed[referencedHash] = deltaRefs.size();
						}

					}
				}
				return allResolvedObjects; 
			}

		}



#pragma region GitPackParser
		GitPackParser::GitPackParser(const std::string& inputPack) :
			_input(inputPack),
			_dataSource(std::make_shared<internal::StringDataSource<>>(inputPack))
		{
			auto map = this->parse_bin_pack();
			if (!map) throw std::runtime_error(map.error());
			_parsedMap = map.value();
		}

#ifdef ZLIB_LOWLEVEL
		auto GitPackParser::parse_bin_pack() -> std::expected<std::unordered_map<std::string, GitObject>, std::string>
		{
			Bytef* rawData = reinterpret_cast<Bytef*>(_input.data());
			auto const numObjects = parse_header().objectCount;
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
				return std::unexpected{ "Failed to initalize zlib decompression" };
			}

			int zlibReturn = Z_OK;
			int bytesRemaining = _input.size() - 20;
			std::unordered_map<std::string, GitObject> objectMap;
			std::queue<DeltarefObject> deltaRefs;
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
				if (result.is_not_deltified()) {
					// Adds object header
					auto objectToCompress = result.compress_input();
					auto hashResult =  utilities::hash_and_save(objectToCompress, false);
					if (!hashResult.has_value()) return std::unexpected(hashResult.error());
					result.hash = hashResult.value();
					std::string compressionError;
					// TODO: Check error here?
					result.compressedData =  utilities::zlib_compressed_str(objectToCompress);
					objectMap[result.hash] = result;
				}
				else if (result.type == ObjectType::REF_DELTA) {
					deltaRefs.push(internal::from_gitobject(result));
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
			size_t const allObjectsCount = objectMap.size() + deltaRefs.size(); 
			if (allObjectsCount != numObjects)
				return std::unexpected("The parsed objects is different form the num of objects in the header");

			auto allResolvedObjects = delta::resolve_delta_refs(objectMap, deltaRefs);
			if (allResolvedObjects.size() != allObjectsCount)
				return std::unexpected("There is a dicrepancy between the objects that should be parsed");

			return allResolvedObjects;
		}

		//TODO:: USe compress function from zlib
#else 
		//TODO::USe zlib compress 
		auto GitPackParser::parse_bin_pack() -> std::expected<std::unordered_map<std::string, GitObject>, std::string>
		{
			return std::expected<std::unordered_map<std::string, GitObject>, std::string>();
	}

#endif // DEBUG

		auto GitPackParser::begin() const

		{
			return _parsedMap.begin();
		}

		auto GitPackParser::end() const
		{
			return _parsedMap.end();
		}


		auto const& GitPackParser::map() const
		{
			return _parsedMap;
		}
		bool GitPackParser::has(std::string sha) const
		{
			return map().contains(sha);
		}

		PackHeader GitPackParser::parse_header()
		{
			auto res = extract_packHeader(this->_input);
			this->_dataSource->advanceN( 20);
			return res;
		}

		std::string GitPackParser::parse_hash_20()
		{
			return internal::binary_sha_to_hex(_dataSource);
		}

		PackObjectHeader GitPackParser::parse_objectHeader()
		{
			char currentByte = _dataSource->advance();
			size_t bytesParsed = 1;
			size_t currentSize = 0;
			const char msbMask = 0x80;
			const char typeMask = 0x70;
			const char last4Mask = 0x0F;
			const char last7Mask = 0x7F;
			char typeChar = (typeMask & currentByte) >> 4;
			auto objectType = static_cast<ObjectType>(typeChar);

			//bool keepParsing = (msbMask & currentByte) != 0;
#pragma region Parse Variable Length size
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
#pragma endregion

			return PackObjectHeader{
				.decompressedSize = currentSize,
				.bytesParsed = bytesParsed,
				.type = objectType };
		}

#pragma endregion

		/**
		* Write the user files in the currentPath
		* @param objects: the objects map from sha to the GitObject
		* @param currentPath: A path to write the object
		* @param object : Target Object to be written
		* @precondiiton : The path already exists before calling the function
		*
		*/
		void checkout_gitobject(
			const std::unordered_map<std::string, GitObject>& objects,
			const std::filesystem::path& currentPath,
			const GitObject& objectToWrite) {
			if (objectToWrite.type == ObjectType::BLOB)/*Base case*/ {
				std::ofstream(currentPath, std::ios::binary) << objectToWrite.uncompressedData;
				return;
			}
			if (objectToWrite.type == ObjectType::TREE) {
				const auto& d = objectToWrite.uncompressedData;
				size_t current = 0;
				/*TreeFile : [TreeEnry][\n[TreeEntry]]*
				* TreeEntry = "{perm} {name}\0{}hash"
				*/
				auto nextSpace = d.find(' ');
				auto nextNull = d.find('\0');
				//TODO :Refactor line by line
				while (nextSpace != std::string::npos && nextNull != std::string::npos) {
					auto filename = d.substr(nextSpace + 1, nextNull - nextSpace - 1);
					auto hash20 = d.substr(nextNull + 1,  constants::sha1Size);
					auto hash40 =  utilities::binary_sha_to_hex(hash20);
					if (objects.count(hash40) != 0) {
						auto nextObject = objects.at(hash40);
						auto newPath = currentPath / filename;
						if (nextObject.type == ObjectType::TREE) {
							std::filesystem::create_directory(newPath);
						}
						checkout_gitobject(objects, newPath, nextObject);
					}
					else
					{
						std::println(std::cerr, "Trying to write a hash:\t {} that is not not in the map! ", hash40);
					}
					nextSpace = d.find(' ', nextNull +  constants::sha1Size + 1);
					nextNull = d.find('\0', nextNull +  constants::sha1Size + 1);
				}
			}
		}

		std::expected<std::string, std::string> clone(
			std::string const url, 
			std::filesystem::path const& beginPath)
		{
			using std::unexpected;

			auto head = clone::get_head(url);
			auto packString = clone::fetch_packfile<char>(url, head);
			std::filesystem::path targetPath = beginPath;
			std::ofstream("/tmp/bin_pack", std::ios::binary) << packString;

			std::ifstream file(packString, std::ios::in | std::ios::binary);
			std::ostringstream oss{};
			oss << file.rdbuf();
			std::string finalPack = oss.str();

			try
			{
				auto const parser = GitPackParser(packString);
				int res =  init();
				if (res == EXIT_FAILURE) return std::unexpected("Failure in init");

				for (const auto& [hash, object] : parser)
				{
					auto objectPath =  utilities::create_directories_and_get_path_from_hash(hash, targetPath);
					std::ofstream(objectPath, std::ios::binary) << object.compressedData;

				}
				if (!parser.map().contains(head.sha()))
				{
					return std::unexpected(
						std::format(
							"Cannot find this head sha \t {}  in those parsed OBjects!!",
							head.sha()));
				}
				auto headObject = parser.map().at(head.sha());
				if (headObject.type != ObjectType::COMMIT)
				{
					return unexpected("Head object eas not a commit!");
				}
				auto headTreeSha = headObject.uncompressedData.substr(
					 constants::hardCodedCommitVals::commitContentStart.size(),
					 constants::sha1Size * 2
				);
				if (!parser.has(headTreeSha))
				{
					return unexpected(
						std::format(
							"Cannot find this heads'tree \t{} in the parsed objects",
							headTreeSha
						)
					);
				}
				auto headTreeObject = parser.map().at(headTreeSha);
				if (headTreeObject.type != ObjectType::TREE)
				{
					return unexpected("Head tree object should be a TREE");
				}
				checkout_gitobject(parser.map(), targetPath, headTreeObject);
				return {};
			}
			catch (std::runtime_error& e)
			{
				return std::unexpected(e.what());
			}



		}

}
	}

