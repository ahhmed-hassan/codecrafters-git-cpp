#include "infrastructure.h"
#include <format>
#include <ranges>
#include <iostream>
#include <string>
#include <sstream>
#include <climits>

#include <cpr/cpr.h>
namespace clone
{
	namespace internal
	{
		bool ObjectHeader::is_deltified() const
		{
			return (this->type == ObjectType::REF_DELTA ||
				this->type == ObjectType::OFS_DELTA) &&
				this->deltaOffsetOrBaseSha.has_value() /*Just for cosistency*/;
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

		void process_git_object(ObjectType type, packstring const& data)
		{
		}

		ObjectHeader parse_object_header_beginning_at(packstring const& packData, size_t startOffset)
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
				//header.offsetDelta = deltaOffset; 
				header.deltaOffsetOrBaseSha = deltaOffset; 

			}
			else if (header.type == ObjectType::REF_DELTA)
			{
				if (pos + 20 > packData.size()) throw std::runtime_error("Insufficient data for base ref"); 
				//header.baseRefSHA = packData.substr(pos, 20); 
				header.deltaOffsetOrBaseSha = packData.substr(pos, 20); 
				pos += 20; 
			}
			
			header.headerBytes = pos - startOffset;  //So that subsequent parsing calls know where the parsing of this object ended
			return header; 
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
}