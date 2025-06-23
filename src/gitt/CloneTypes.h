#pragma once
#include <string>
#include <vector>
#include <variant>

namespace commands
{
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
            bool is_not_deltified() const;
            std::string get_type_for_non_deltiifed() const;
        };

        struct PackObjectHeader
        {
            size_t decompressedSize;
            size_t bytesParsed{};
            ObjectType type{};
            bool is_not_deltified() const;

        };


        struct PackHeader
        {
            char magic[4]{};
            uint32_t version{};
            uint32_t objectCount{};
        };

        struct CopyInstruction
        {
            // Copy instructions
            // Number of offset bytes gleaned from bits 0-3, can be 1,2,4,8
            // Then count the bytes in little endian format
            size_t offset{};
            // Number of size bytes gleaned from bits 4-6, can be 1,2,4
            size_t size{};
            std::string apply_delta(std::string const& reference) const;
        };

        struct InsertInstruction
        {
            size_t numBytesToInsert{};
            std::string dataToInsert{};
        };
        using DeltaRefInstruction = std::variant<CopyInstruction, InsertInstruction>;
    };
}