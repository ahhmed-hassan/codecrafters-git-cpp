#pragma once
#include <string>
#include <vector>

#include "infrastructure.h"
#include <cstdint>
namespace clone
{
 
    /*auto refVisitor = overload{
        [](const GitRef& gitref) {return std::make_tuple(gitref.name, gitref.object_id); },
        [](HeadRef&& head) {return std::make_tuple(head.ref.name, head.ref.object_id, head.capabilities); }
    };*/
    struct PackHeader
    {
        char magic[4]{};
        uint32_t version{};
        uint32_t objectCount{};
    };
    HeadRef get_head(std::string const& url); 
    packstring fetch_packfile(const std::string& url,  HeadRef head);
    PackHeader extract_packHeader(packstring const& packData);
    void process_packfile(packstring const& packData);

}