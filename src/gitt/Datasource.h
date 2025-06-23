#pragma once
#include <string>
#include <fstream>
namespace commands
{
    namespace clone
    {
        namespace internal
        {
            template <typename CharT = char>
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


            //TODO: implement begin and end to make an interator sentintel pair here. 
            template <typename CharT = char>
            class StringDataSource : public DataSource<CharT> {
            public:
                StringDataSource(const std::basic_string<CharT>& input);
                CharT advance() override;
                CharT peek()  override;
                bool isAtEnd() override;
                CharT const* data() const;
            private:
                std::basic_string<CharT> _input;
                size_t _index = 0;
            };

            template <typename CharT = char>
            class FileDataSource : public DataSource<CharT> {
            public:
                FileDataSource(const std::basic_string<CharT>& filename);
                ~FileDataSource();
                CharT advance() override;
                CharT peek()  override;
                bool isAtEnd() override;
            private:
                std::basic_ifstream<CharT> _ifstream{};
                bool _isAtEnd = false;
            };
        }
    }
}