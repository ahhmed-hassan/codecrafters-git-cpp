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
    enum class ObjectType
    {
        COMMIT = 1,
        TREE = 2,
        BLOB = 3,
        TAG = 4,
        OFS_DELTA = 6,
        REF_DELTA = 7,
    };
    struct GitObject {
        std::string hash;
        ObjectType type;
        std::string compressedData;
        std::string uncompressedData;
    };
}
