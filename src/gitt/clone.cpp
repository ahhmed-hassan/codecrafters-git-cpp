#include <string>
#include <asio.hpp>
#include <cpr/cpr.h>
#include <iostream>

#include "clone.h"

namespace clone
{
    HeadRef get_head(std::string const& url)
    {
        auto response = internal::get_refs_info(url);
        auto refs = clone::internal::parse_refs_info(response);
        auto headRef = std::get<HeadRef>(*refs.begin());
        return headRef;
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
        std::string body = internal::build_negotiation_body(head);
        body = std::format("0032want {}\n00000009done\n", head.ref.object_id);

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