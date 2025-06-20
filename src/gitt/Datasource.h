#pragma once
#include <string>
#include <fstream>
namespace clone
{
    namespace internal
    {
        
        class DataSource {
        public:
            virtual char advance() = 0;
            virtual char peek() = 0;
            char previous();
            virtual bool isAtEnd() = 0;
            // Output size is always n. Will return EOF
            // characters if advancing past the end.
            // Does not do any checking for isAtEnd().
            // User is responsible for knowing if this is reasonable.
            std::string advanceN(size_t n);
        protected:
            char _previous = '\0';
        };
        
        class StringDataSource : public DataSource {
        public:
            StringDataSource(const std::string& input);
            char advance() override;
            char peek() override;
            bool isAtEnd() override;
        private:
            std::string _input;
            size_t _index = 0;
        };
        
        class FileDataSource : public DataSource {
        public:
            FileDataSource(const std::string& filename);
            ~FileDataSource();
            char advance() override;
            char peek() override;
            bool isAtEnd() override;
        private:
            std::ifstream _ifstream{};
            bool _isAtEnd = false;
        };
    }
}