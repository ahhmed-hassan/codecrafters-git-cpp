#pragma once
#include <string>
#include <vector>
#include <expected>  
#include <variant>

#include "infrastructure.h"
namespace clone
{
    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload(Ts...) -> overload<Ts...>;
    struct GitRef 
    {
        std::string object_id;
        std::string name;
    };
    struct HeadRef
    {
        GitRef ref{}; 
        std::vector<std::string> capabilities{}; 
    };
    using Ref = std::variant<GitRef, HeadRef>;
    /*auto refVisitor = overload{
        [](const GitRef& gitref) {return std::make_tuple(gitref.name, gitref.object_id); },
        [](HeadRef&& head) {return std::make_tuple(head.ref.name, head.ref.object_id, head.capabilities); }
    };*/
    std::string get_refs_info(const std::string& url= "https://github.com/i27ae15/git.git");
    std::vector<Ref> parse_refs_info(std::string const& getResponse);
    HeadRef get_head(std::string const& url); 
    std::string build_negotiation_body(HeadRef head);
    std::string fetch_packfile(const std::string& url,  HeadRef head); 
    std::string extract_packFile(std::string const& packData); 

}