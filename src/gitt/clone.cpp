#include <string>
#include <regex>
#include <asio.hpp>
#include <cpr/cpr.h>
#include <iostream>
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
        std::println(std::cout, "GET response\n--------------\n{}", response); 
        auto refs = clone::parse_info_refs(response);
        auto headRef = std::get<HeadRef>(*refs.begin());
        std::println(std::cout, "HEAD SHA\n----------\n{}", headRef.ref.object_id);
        return headRef;
    }

    std::string build_negotiation_body(HeadRef head) {
        // Format want/done commands with pkt-line encoding
        auto capsRange = std::ranges::join_with_view(head.capabilities, ' ');
        auto caps = std::ranges::fold_left(capsRange, std::string{}, std::plus());
        std::string want = std::format("want {} {}\n", head.ref.object_id, caps);
        std::string done = "done\n";

        // Convert to pkt-line (4-byte hex length + data)
        auto format_pkt_line = [](const std::string& data) {
            return std::format("{:04x}{}", data.size() + 4, data);
            };

        return format_pkt_line(want) + format_pkt_line(done) + "0000"; // Flush packet
    }

    std::string fetch_packfile(const std::string& url,  HeadRef head) {
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
        std::string body = std::format("0032want {}\n00000009done\n", head.ref.object_id);

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
        return r.text;
    }

    std::string extract_packFile(std::string const& packData)
        {
      
            std::string packfile;
            size_t pos = 0;

            while (pos < packData.size()) {
                // Read pkt-line header
                std::string header = packData.substr(pos, 4);
                pos += 4;

                uint32_t length = std::stoul(header, nullptr, 16);
                if (length == 0) break; // Flush packet

                // Extract data (length includes 4-byte header)
                std::string data = packData.substr(pos, length - 4);
                pos += length - 4;

                // Handle sideband (first byte is band ID)
                if (data[0] == '\x01') { // Main data
                    packfile += data.substr(1);
                }
                else if (data[0] == '\x02') { // Progress messages
                    std::cerr << data.substr(1);
                }
            }

            return packfile;
        
    }

}