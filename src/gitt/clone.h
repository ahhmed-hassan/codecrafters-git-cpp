#pragma once
#include <string>
#include <vector>

#include "infrastructure.h"
#include "Datasource.h"
#include <cstdint>
namespace clone
{
 
#define ZLIB_LOWLEVEL

    //HeadRef get_head(std::string const& url); 
    //packstring fetch_packfile(const std::string& url,  HeadRef head);
    //PackHeader extract_packHeader(packstring const& packData);
    //void process_packfile(packstring const& packData);
    class GitPackParser {
    public:
        GitPackParser(const std::string& inputPack);
        auto parse_bin_pack() ->
            std::expected<std::unordered_map<std::string, GitObject>, std::string>;

        auto begin() const; 
        auto end() const; 
    private:
        std::shared_ptr<internal::DataSource<>> _dataSource;
        std::string _input;
        std::unordered_map<std::string, GitObject> _parsedMap{};

    private:
        PackHeader parse_header();
        std::string parse_hash_20();
        PackObjectHeader parse_objectHeader();
        
    };

 

}