#include "infrastructure.h"
#include "utilities.h"
#include "commands.h"
#include <format>
#include <ranges>
#include <iostream>
#include <string>
#include <sstream>
#include <climits>
#include "zstr.hpp"
#include <cpr/cpr.h>
#include "Datasource.h"
namespace clone
{
	namespace internal
	{
		bool ObjectHeader::is_deltified() const
		{
			return is_ref_delta() || is_offset_delta();
		}
		bool ObjectHeader::is_ref_delta() const
		{
			return deltaOffsetOrBaseSha.has_value() &&
				std::holds_alternative<packstring>(deltaOffsetOrBaseSha.value());
		}
		bool ObjectHeader::is_offset_delta() const
		{
			return deltaOffsetOrBaseSha.has_value() &&
				std::holds_alternative<uint64_t>(deltaOffsetOrBaseSha.value());
		}
		packstring ObjectHeader::ref() const
		{
			assert(is_ref_delta());
			return std::get<packstring>(deltaOffsetOrBaseSha.value());
		}
		uint64_t ObjectHeader::offset() const
		{
			assert(is_offset_delta());
			return std::get<uint64_t>(deltaOffsetOrBaseSha.value());
		}
		std::string build_negotiation_body(HeadRef head)
		{

			// Format want/done commands with pkt-line encoding
			auto capsRange = std::ranges::join_with_view(head.capabilities, ' ');
			//auto caps = std::ranges::fold_left(capsRange, std::string{}, std::plus());
			std::string want = std::format("want {} \n", head.sha());
			std::string done = "done\n";

			// Convert to pkt-line (4-byte hex length + data)
			auto format_pkt_line = [](const std::string& data) {
				return std::format("{:04x}{}", data.size() + 4, data);
				};

			return format_pkt_line(want) + format_pkt_line(done) + "0000"; // Flush packet
		}

		std::vector<clone::Ref> parse_refs_info(std::string const& getResponse)
		{
			std::string const header = "0000";
			std::vector<Ref> refs;
			std::istringstream iss(getResponse);
			std::string line;
			//Consume the first line
			std::getline(iss, line);

			while (std::getline(iss, line)) {
				// Skip flush packets and service header
				if (line == header) continue;

				// Extract pkt-line length (first 4 chars as hex)
				uint32_t length;
				std::stringstream len_ss;
				len_ss << std::hex << line.substr(0, 4);
				len_ss >> length;

				if (length == 0)
				{
					if (line.size() > 4)
					{
						line = line.substr(8);
					}
					else
						continue;
				}
				else
					line = line.substr(4, length - 4);



				// Parse into GitRef
				GitRef ref;
				size_t first_space = line.find(' ');
				size_t null_pos = line.find('\0');

				ref.object_id = line.substr(0, first_space);
				ref.name = line.substr(first_space + 1,
					(null_pos != std::string::npos) ? null_pos - first_space - 1 : std::string::npos);

				// Parse capabilities (only for HEAD)
				if (/*ref.name == "HEAD" &&*/ null_pos != std::string::npos || line.contains("HEAD")) {
					HeadRef headRef{ .ref = ref };
					std::string caps_str = line.substr(null_pos + 1);
					std::istringstream caps_iss(caps_str);
					std::string cap;
					while (std::getline(caps_iss, cap, ' ')) {
						headRef.capabilities.push_back(cap);
					}
					refs.push_back(headRef); continue;
				}

				refs.push_back(ref);
			}

			return refs;
		}

		std::string objecttype_to_str(ObjectType objType)
		{
			switch (objType)
			{
			case ObjectType::COMMIT:
				return "commmit";
			case ObjectType::BLOB:
				return "blob";
			case ObjectType::TREE:
				return "tree";
			case ObjectType::REF_DELTA:
				return "ref_delta";
			case ObjectType::TAG:
				return "tag";
			case ObjectType::OFS_DELTA:
				return "ofs_delta";
			default: throw std::runtime_error("Not a known object Type");

			}
		}

		bool is_not_deltified(ObjectType objType)
		{
			return objType == ObjectType::COMMIT ||
				objType == ObjectType::TREE ||
				objType == ObjectType::BLOB;
		}

		void process_git_object(bool is_deltified, packstring const& data)
		{
		}

		ObjectHeader get_object_header_beginning_at(packstring const& packData, size_t startOffset)
		{
			if (startOffset >= packData.size()) throw std::runtime_error("Start offset beyon data size");
#pragma region Constants
			uint8_t const rightMostThreeBits = 0x7;
			[[maybe_unused]] uint16_t const rightMostFoursBits = 0x0F;
			size_t const initialShiftSize = 4;
			size_t const nextShifSize = 7;
#pragma endregion

			size_t pos = startOffset;
			const auto size = packData.size();
			ObjectHeader header{};
			unsigned char byte = packData[pos++];


#pragma region Type and Variable Length size parsing
			//Extract Type and initial size
			header.type = static_cast<ObjectType>((byte >> 4) & rightMostThreeBits);
			uint64_t objSize = byte & 0x0F;

			//Variable length size
			int currentShiftSize = nextShifSize;
			auto hasMostSignficantBit = [](unsigned char byte) {return byte & 0x80;  };

			while (hasMostSignficantBit(byte))
			{
				if (pos >= packData.size()) throw std::runtime_error("Unexpected end of data");
				byte = packData[pos++];
				unsigned char curLast7Bits = (byte & 0x7F /*All Ones Except the MSB*/);
				objSize |= static_cast<uint64_t>(curLast7Bits << currentShiftSize);
				currentShiftSize += nextShifSize;
			}
			header.decompressedSize = objSize;

#pragma endregion

			//If it was not a ref delta or ofs delta then we only needed the size parsed above
			if (header.type == ObjectType::OFS_DELTA)
			{
				int shift{};
				uint64_t deltaOffset = 0;
				do
				{
					if (pos >= packData.size()) throw std::runtime_error("UnExpected end of offset data");

					byte = packData[pos++];
					unsigned char curLast7BitsForDelta = (byte & 0x7F /*All Ones Except the MSB*/);
					deltaOffset |= static_cast <uint64_t>(curLast7BitsForDelta << shift);

				} while (hasMostSignficantBit(byte));
				header.deltaOffsetOrBaseSha = deltaOffset;

			}
			else if (header.type == ObjectType::REF_DELTA)
			{
				if (pos + 20 > packData.size()) throw std::runtime_error("Insufficient data for base ref");
				header.deltaOffsetOrBaseSha = packData.substr(pos, 20);
				pos += 20;
			}

			header.headerBytes = pos - startOffset;  //So that subsequent parsing calls know where the parsing of this object ended
			return header;
		}

		std::string apply_ref_delta(const packstring& baseRef, const packstring& delta_data)
		{
			
				// 1. Convert binary SHA to hex
				auto bin_sha_to_hex = [](const packstring& bin_sha) -> std::string {
					std::ostringstream oss;
					for (unsigned char c : bin_sha) {
						oss << std::hex << std::setw(2) << std::setfill('0')
							<< static_cast<int>(c);
					}
					return oss.str();
					};

				std::string base_sha = bin_sha_to_hex(baseRef);

				// 2. Get base object content using your existing utilities
				//BUG: Not working now return the object here
				auto catResult = commands::cat( "-p", base_sha);
				if (!catResult) throw std::runtime_error(catResult.error());
				std::string base_obj = catResult.value();
				// 3. Apply delta instructions
				size_t delta_pos = 0;

				// Read base and result sizes (variable-length integers)
				auto read_varint = [&]() -> uint64_t {
					uint64_t value = 0;
					int shift = 0;
					unsigned char byte;
					do {
						if (delta_pos >= delta_data.size()) {
							throw std::runtime_error("Unexpected end of delta data");
						}
						byte = delta_data[delta_pos++];
						value |= static_cast<uint64_t>(byte & 0x7F) << shift;
						shift += 7;
					} while (byte & 0x80);
					return value;
					};

				uint64_t base_size = read_varint();
				uint64_t result_size = read_varint();

				if (base_size != base_obj.size()) {
					throw std::runtime_error("Base size mismatch");
				}

				std::string result;
				result.reserve(result_size);

				// Process delta instructions
				while (delta_pos < delta_data.size()) {
					unsigned char inst = delta_data[delta_pos++];

					// Add instruction
					if ((inst & 0x80) == 0) {
						uint64_t length = inst;
						if (length > 0) {
							if (delta_pos + length > delta_data.size()) {
								throw std::runtime_error("Invalid add instruction");
							}
							result.append(reinterpret_cast<const char*>(
								&delta_data[delta_pos]), length);
							delta_pos += length;
						}
					}
					// Copy instruction (simplified for initial implementation)
					else {
						// For now, just copy the entire base object
						// (We'll implement proper copy later)
						result.append(base_obj);
					}
				}

				if (result.size() != result_size) {
					throw std::runtime_error("Result size mismatch");
				}

				return result;
			};
		
		
		void process_non_deltified(ObjectHeader const& header, packstring const& data)
		{
			if (header.is_deltified()) throw std::runtime_error("This function shall be only called or non deltified objects");

			std::string dataStr(data.begin(), data.end());
			
			auto dataStream = std::istringstream(dataStr);
			zstr::istream fileStream(dataStream);
			
			//To Be compatible with hash_and_save funciton we should for ease of use decompress the content in the pack string, then computes the header + the content
			//and give it to the function to compute it as any content to hash. (Please note we are not computing anything here. The data is alreday ready. We are just populating to the database)
			std::string const uncompressedData{ std::istream_iterator<char>(fileStream), std::istream_iterator<char>() };

			std::string meta_data = internal::objecttype_to_str(header.type) + " " + std::to_string(uncompressedData.size()) + '\0';

			commands::utilities::hash_and_save(meta_data + uncompressedData, true);

		}

		std::string get_refs_info(const std::string& url)
		{
			std::string full_url = std::format("{}/info/refs?service=git-upload-pack", url);
			cpr::Response r = cpr::Get(cpr::Url{ full_url });

			if (r.status_code != 200) {
				throw std::runtime_error("HTTP request failed");
			}
			return r.text;
		}
	}

	bool GitObject::is_not_deltified() const { return internal::is_not_deltified(type); }; 
	bool PackObjectHeader::is_not_deltified() const { return internal::is_not_deltified(type); };
	std::string GitObject::get_type_for_non_deltiifed() const {
		assert(is_not_deltified()); 
		return internal::objecttype_to_str(type);
	}
}