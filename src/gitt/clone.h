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
   
    HeadRef get_head(std::string const& url); 
    using packstring = std::basic_string<unsigned char>; 
    packstring fetch_packfile(const std::string& url,  HeadRef head);
    std::string extract_packFile(packstring const& packData);

}