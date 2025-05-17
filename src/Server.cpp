#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include<sstream>
#include <algorithm>
#include "gitt/commands.h"

//#define DEBUG
#ifndef DEBUG
int main(int argc, char *argv[]){
    std::vector<std::string> args(argv, argv + argc);
#else
#include<sstream>
int main() {
    std::string input;
    std::getline(std::cin, input);

    std::istringstream iss(input);
    std::vector<std::string> args((std::istream_iterator<std::string>(iss)),
        std::istream_iterator<std::string>());
    args.insert(args.begin(), "notImportant");
    size_t argc = args.size();

#endif // DEBUG




    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here!\n";

    
     
     if (argc < 2) {
         std::cerr << "No command provided.\n";
         return EXIT_FAILURE;
     }
    
     std::string command = args[1];



     if (command == "init") {
         return commands::init();
     }
     else if (command == "cat-file") {
         std::string option = args[2]; 
         std::string arg = args[3]; 
         return commands::cat(args[2], args[3]);
     }
     else if (command == "hash-object")
     {
         
         std::string arg = args.back(); 
         bool writeOption = std::ranges::find(args, "-w")!= args.end();
         return commands::hash(arg, writeOption, true); 
     }
     else if (command == "ls-tree")
     {
         std::string arg = args.back(); 
         bool namesOnly = std::ranges::find(args, "--name-only") != args.end(); 
         return commands::ls_tree(arg, namesOnly); 
     }
     else if (command == "write-tree")
     {
         return commands::write_tree();
     }
     else if (command == "commit-tree")
     {
         std::string treeHash = *std::next(std::ranges::find(args,"commit-tree"));
         std::optional<std::string> parentTreeHash{ std::nullopt };
         if (auto parentOptionIt = std::ranges::find(args, "-p"); parentOptionIt != args.end())
             parentTreeHash = *std::next(parentOptionIt);
         std::optional<std::string> msg{ std::nullopt };
         if (auto msgOptionIt = std::ranges::find(args, "-m"); msgOptionIt != args.end())
         {
             auto reconstructedQuotedMsg = std::ranges::fold_left(std::next(msgOptionIt), args.end(), std::string{},
                 [](std::string const& left, std::string const& right) {return left + (left.empty() ? "" : " ") + right; });
             //std::string  = *std::next(msgOptionIt);
             msg = reconstructedQuotedMsg.starts_with('\"') and reconstructedQuotedMsg.ends_with('\"') ?
                 reconstructedQuotedMsg.substr(1, reconstructedQuotedMsg.size() - 2) :
                 reconstructedQuotedMsg;
         }

             
         return commands::commmit(treeHash, parentTreeHash, msg); 
     }
     else {
         std::cerr << "Unknown command " << command << '\n';
         return EXIT_FAILURE;
     }
    
     //return EXIT_SUCCESS;
}
