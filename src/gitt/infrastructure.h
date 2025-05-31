#pragma once
#include "Ref.h"
#include <string>
namespace clone
{
	namespace internal
	{
		std::string build_negotiation_body(HeadRef head);
		std::string get_refs_info(const std::string& url = "https://github.com/i27ae15/git.git");
		std::vector<Ref> parse_refs_info(std::string const& getResponse);
	}
}