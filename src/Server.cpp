#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
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

    // Uncomment this block to pass the first stage
    //
     if (argc < 2) {
         std::cerr << "No command provided.\n";
         return EXIT_FAILURE;
     }
    
     std::string command = args[1];
    
     if (command == "init") {
         return commands::init_command();
     }
     else if (command == "cat-file") {
         std::string option = args[2]; 
         std::string arg = args[3]; 
         return commands::cat_command(args[2], args[3]);
     }
     else if (command == "hash-object")
     {
         
         std::string arg = args.back(); 
         bool writeOption = std::ranges::find(args, "-w")!= args.end();
         return commands::hash_command(arg, writeOption); 
     }
     else {
         std::cerr << "Unknown command " << command << '\n';
         return EXIT_FAILURE;
     }
    
     //return EXIT_SUCCESS;
}
