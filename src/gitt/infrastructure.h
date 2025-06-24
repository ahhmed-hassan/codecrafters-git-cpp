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

namespace commands
{
	namespace clone
	{
		namespace internal
		{
			std::string build_negotiation_body(HeadRef head);
			std::string get_refs_info(const std::string& url = "https://github.com/i27ae15/git.git");
			std::vector<Ref> parse_refs_info(std::string const& getResponse);

			std::string objecttype_to_str(ObjectType objType);
			bool is_not_deltified(ObjectType objType);
			
		}
	}
}