#pragma once
#include <string>
#include <vector>
#include <variant>

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
        std::string sha() const;
    };
    using Ref = std::variant<GitRef, HeadRef>;
   
}
