#pragma once
#include "Ref.h"
#include <string>
#include <cstdint>
#include <optional>
namespace clone
{
	using packstring = std::basic_string<unsigned char>;
	using packstring_view = std::basic_string_view<unsigned char>;
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
		struct OffsetDelta { uint64_t offset{}; };
		struct BaseRefSHA { packstring baseSha{}; };
		struct ObjectHeader
		{
			
			ObjectType type{}; 
			uint64_t decompressedSize{}; 
			size_t headerBytes{}; 
			using maybeOffsetOrBaseSha = std::optional<std::variant<uint64_t, packstring>>; 
			maybeOffsetOrBaseSha deltaOffsetOrBaseSha{}; 
			//std::optional<uint64_t> offsetDelta{};  //OFS_Delta
			//std::optional<packstring> baseRefSHA{}; //RefDelta

			bool is_deltified() const; 
		};

		void process_git_object(ObjectType type, packstring const&); 
		ObjectHeader parse_object_header_beginning_at(packstring const& packData, size_t startOffset = 0); 
	}
}