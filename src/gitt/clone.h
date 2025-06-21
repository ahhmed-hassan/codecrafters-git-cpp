#pragma once
#include <string>
#include <vector>

#include "infrastructure.h"
#include "Datasource.h"
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
    //HeadRef get_head(std::string const& url); 
    //packstring fetch_packfile(const std::string& url,  HeadRef head);
    //PackHeader extract_packHeader(packstring const& packData);
    void process_packfile(packstring const& packData);
    class GitPackParser {
    public:
        GitPackParser(const std::string& inputPack);
        auto parseBinPack() ->
            std::expected<std::unordered_map<std::string, GitObject>, std::string>;
    private:
        std::shared_ptr<internal::DataSource<>> _dataSource;
        std::string _input;
        std::unordered_map<std::string, GitObject> _parsedMap{};

    private:
        PackHeader parseHeader();
        std::string parseHash20();
        PackObjectHeader parseObjectHeader();
        std::pair<GitObject, size_t> parseNextObject(Bytef* rawData, z_stream* stream, int& zlibReturn);
        void init_map(); 
    };

 

}