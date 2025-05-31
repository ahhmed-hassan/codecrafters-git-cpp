#include "infrastructure.h"
#include <format>
#include <ranges>
#include <iostream>
#include <string>
#include <sstream>

#include <cpr/cpr.h>
namespace clone
{
	namespace internal
	{
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