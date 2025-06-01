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



	packstring fetch_packfile(const std::string& url, HeadRef head) {
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
		return packstring{ r.text.begin(), r.text.end() };
	}

	struct PackHeader
	{
		char magic[4]{}; 
		uint32_t version{}; 
		uint32_t objectCount{}; 
	};
	std::string extract_packFile(packstring const& packData)
	{
		if (packData.size() < 12) throw std::runtime_error("Invlid packfile size");
		PackHeader header{};
		std::memcpy(&header, packData.data() + sizeof("0008NAK"), sizeof(PackHeader));
		header.version = ntohl(header.version); 
		header.objectCount = ntohl(header.objectCount);

#pragma region verification
		if(std::memcmp(header.magic, "PACK", sizeof("PACK") != 0))
			throw std::runtime_error("Invalid packfile singature");

		if (header.version != 2 && header.version != 3)
			throw std::runtime_error("Unsupported packfile version");
#pragma endregion 


		return {};
	}

}