#include <string>
#include <regex>
#include <asio.hpp>
#include <cpr/cpr.h>
#include "clone.h"

namespace clone
{
    std::string clone::get_info_refs(const std::string& url)
    {
        std::string full_url = std::format("{}/info/refs?service=git-upload-pack", url);
        cpr::Response r = cpr::Get(cpr::Url{ full_url });

        if (r.status_code != 200) {
            throw std::runtime_error("HTTP request failed");
        }
        return r.text;
    }

    std::vector<clone::Ref> clone::parse_info_refs(std::string const& getResponse)
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
            //if (line.find("# service=") != std::string::npos) continue;

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

    std::string clone::get_head_sha(std::string const& url)
    {
        auto response = get_info_refs(url);
        auto refs = clone::parse_info_refs(response);
        auto headRef = std::get<HeadRef>(*refs.begin());
        return headRef.ref.object_id;
    }

    HeadRef get_head(std::string const& url)
    {
        auto response = get_info_refs(url);
        auto refs = clone::parse_info_refs(response);
        auto headRef = std::get<HeadRef>(*refs.begin());
        return headRef;
    }

    std::string build_negotiation_body(HeadRef head) {
        // Format want/done commands with pkt-line encoding
        auto capsRange = std::ranges::join_with_view(head.capabilities, ' ');
        auto reconstructedQuotedMsg = std::ranges::fold_left(capsRange, std::string{}, std::plus());
        std::string want = std::format("want {} {}\n", head.ref.object_id, reconstructedQuotedMsg);
        std::string done = "done\n";

        // Convert to pkt-line (4-byte hex length + data)
        auto format_pkt_line = [](const std::string& data) {
            return std::format("{:04x}{}", data.size() + 4, data);
            };

        return format_pkt_line(want) + format_pkt_line(done) + "0000"; // Flush packet
    }

    std::string fetch_packfile(const std::string& url,  HeadRef head) {
        std::string upload_pack_url = std::format("{}/git-upload-pack", url);
        std::string body = build_negotiation_body(head);

        cpr::Response r = cpr::Post(
            cpr::Url{ upload_pack_url },
            cpr::Body{ body },
            cpr::Header{
                {"Content-Type", "application/x-git-upload-pack-request"},
                {"User-Agent", "my-git-client/1.0"}
            } 
            
        );

        if (r.status_code != 200) {
            throw std::runtime_error("Failed to fetch packfile");
        }
        return r.text;
      
    }

}