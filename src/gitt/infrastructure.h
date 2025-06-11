#pragma once
#include "Ref.h"
#include <string>
#include <cstdint>
#include <optional>
namespace clone
{
	using packstring = std::basic_string<unsigned char>;
	namespace internal
	{
		std::string build_negotiation_body(HeadRef head);
		std::string get_refs_info(const std::string& url = "https://github.com/i27ae15/git.git");
		std::vector<Ref> parse_refs_info(std::string const& getResponse);
		enum class ObjectType
		{
			COMMIT = 1, 
			TREE = 2, 
			BLOB = 3, 
			TAG = 4, 
			OFS_DELTA= 6, 
			REF_DELTA = 7, 
		};

		struct ObjectHeader
		{
			ObjectType type{}; 
			uint64_t decompressedSize{}; 
			size_t headerBytes{}; 
			std::optional<uint64_t> offsetDelta{};  //OFS_Delta
			std::optional<packstring> baseRef{}; //RefDelta
		};

		void process_git_object(ObjectType type, packstring const&); 
		ObjectHeader parse_object_header_beginning_at(packstring const& packData, size_t startOffset = 0); 
	}
}