#pragma once
#include "CLoneTypes.h"
#include <string>
#include <cstdint>
#include <optional>
#include <expected>
#include <memory>
#include "Datasource.h"
#include <unordered_map>
#include <zlib.h>

namespace clone
{
	using packstring = std::basic_string<unsigned char>;
	using packstring_view = std::basic_string_view<unsigned char>;

	namespace internal
	{

		
		std::string build_negotiation_body(HeadRef head);
		std::string get_refs_info(const std::string& url = "https://github.com/i27ae15/git.git");
		std::vector<Ref> parse_refs_info(std::string const& getResponse);
	
		std::string objecttype_to_str(ObjectType objType);
		bool is_not_deltified(ObjectType objType); 
		struct ObjectHeader
		{
			
			ObjectType type{}; 
			uint64_t decompressedSize{}; 
			size_t headerBytes{}; 
			using maybeOffsetOrBaseSha = std::optional<std::variant<uint64_t, packstring>>; 
			maybeOffsetOrBaseSha deltaOffsetOrBaseSha{}; 
			bool is_deltified() const; 
			bool is_ref_delta() const; 
			bool is_offset_delta() const; 
			packstring ref() const; 
			uint64_t offset() const; 
		};

		
		void process_git_object(bool is_deltified, packstring const&); 
		ObjectHeader get_object_header_beginning_at(packstring const& packData, size_t startOffset = 0); 

		void process_deltified(ObjectHeader const& header, packstring const& data); 
		/*
		* Populates the non deltified object to the database first then the tree should be constructed later from those new written objects
		* using the already existing utilities for reading blobs and trees 
		*/
		void process_non_deltified(ObjectHeader const& header, packstring const& data); 

		


	
	}
}