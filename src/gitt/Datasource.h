#pragma once
#include <string>
#include <fstream>
namespace clone
{
    namespace internal
    {
        template <typename CharT= char>
        class DataSource {
        public:
            using stringType = std::basic_string<CharT>; 
            virtual CharT advance() = 0;
            virtual CharT peek() = 0;
            CharT previous();
            virtual bool isAtEnd() = 0;
            // Output size is always n. Will return EOF
            // characters if advancing past the end.
            // Does not do any checking for isAtEnd().
            // User is responsible for knowing if this is reasonable.
            stringType advanceN(size_t n);
        protected:
            CharT _previous = '\0';
        };
        
       
        //template <typename CharT>
        class StringDataSource : public DataSource<> {
        public:
            StringDataSource(const std::string& input);
            char advance() override;
            char peek() override;
            bool isAtEnd() override;
        private:
            std::string _input;
            size_t _index = 0;
        };
        
        class FileDataSource : public DataSource<> {
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