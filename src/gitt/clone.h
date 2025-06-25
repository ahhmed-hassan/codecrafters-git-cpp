#pragma once
#include <string>
#include <vector>
#include<filesystem>

#include "infrastructure.h"
#include "Datasource.h"
#include <cstdint>
namespace commands
{
    namespace clone
    {

#define ZLIB_LOWLEVEL

        class GitPackParser {
        public:
            GitPackParser(const std::string& inputPack);

            //auto map() const; 
            auto const& map() const;
            auto begin() const;
            auto end() const;
            bool has(std::string sha) const;
        private:
            auto parse_bin_pack() ->
                std::expected<std::unordered_map<std::string, GitObject>, std::string>;
            std::shared_ptr<internal::DataSource<>> _dataSource;
            std::string _input;
            std::unordered_map<std::string, GitObject> _parsedMap{};

        private:
            PackHeader parse_header();
            std::string parse_hash_20();
            PackObjectHeader parse_objectHeader();

        };

       

    }
}