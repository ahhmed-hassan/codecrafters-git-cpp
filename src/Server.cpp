#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "gitt/commands.h"

//#define DEBUG
#ifndef DEBUG
int main(int argc, char *argv[]){
#else
#include<sstream>
int main() {
    std::string input;
    std::getline(std::cin, input);

    std::istringstream iss(input);
    std::vector<std::string> argv((std::istream_iterator<std::string>(iss)),
        std::istream_iterator<std::string>());
    argv.insert(argv.begin(), "notImportant");
    size_t argc = argv.size();

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
    
     std::string command = argv[1];
    
     if (command == "init") {
         return commands::init_command();
     }
     else if (command == "cat-file") {
         std::string option = argv[2]; 
         std::string arg = argv[3]; 
         return commands::cat_command(argv[2], argv[3]);
     }
     else {
         std::cerr << "Unknown command " << command << '\n';
         return EXIT_FAILURE;
     }
    
     //return EXIT_SUCCESS;
}
