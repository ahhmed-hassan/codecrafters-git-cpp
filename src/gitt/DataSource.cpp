#include "Datasource.h"
namespace clone
{
	namespace internal
	{
		template <typename CharT>
		CharT DataSource<CharT>::previous() {
			return _previous;
		}
		template char DataSource<char>::previous();
		template unsigned char DataSource<unsigned char>::previous();

		template <typename CharT>
		DataSource<CharT>::stringType DataSource<CharT>::advanceN(size_t n) {
			stringType result{};
			for (size_t i = 0; i < n; ++i) {
				result += advance();
			}
			return result;
		}
		template std::basic_string<char> DataSource<char>::advanceN(size_t n); 
		template std::basic_string<unsigned char> DataSource<unsigned char>::advanceN(size_t n); 

		template <typename CharT>
		StringDataSource<CharT>::StringDataSource(const std::basic_string<CharT> & input) : _input(input) {}
		template StringDataSource<char>::StringDataSource(std::basic_string<char> const&);
		template StringDataSource<unsigned char>::StringDataSource(std::basic_string<unsigned char> const&);

		template <typename CharT>
		CharT StringDataSource<CharT>::advance() {
			if (_index >= _input.size()) {
				return EOF;
			}
			this->_previous = _input[_index++];
			return this->_previous;
		}
		template char StringDataSource<char>::advance();
		template unsigned char StringDataSource<unsigned char>::advance();

		template <typename CharT>
		CharT StringDataSource<CharT>::peek()  {
			if (_index >= _input.size()) {
				return EOF;
			}
			return _input[_index];
		}
		template char StringDataSource<char>::peek();
		template unsigned char StringDataSource<unsigned char>::peek();

		template <typename CharT>
		bool StringDataSource<CharT>::isAtEnd() {
			return _index >= _input.size();
		}
		template bool StringDataSource<char>::isAtEnd();
		template bool StringDataSource<unsigned char>::isAtEnd();

		
		template <typename CharT>
		//HACK:Does the conversion make sense? 
		FileDataSource<CharT>::FileDataSource(const std::basic_string<CharT>& filename) : _ifstream(std::string{ filename.begin(), filename.end() }) {
			if (!_ifstream.is_open()) {
				_isAtEnd = true;
			}
		}
		template FileDataSource<char>::FileDataSource(std::basic_string<char> const& );
		template FileDataSource<unsigned char>::FileDataSource(std::basic_string<unsigned char> const&);

		template <typename CharT>
		FileDataSource<CharT>::~FileDataSource() {
			if (_ifstream.is_open()) {
				_ifstream.close();
			}
		}
		template FileDataSource<char>::~FileDataSource();
		template FileDataSource<unsigned char>::~FileDataSource();

		template <typename CharT>
		CharT FileDataSource<CharT>::advance() {
			CharT ch;
			if (!_ifstream.get(ch)) {
				_isAtEnd = true;
				return EOF;
			}
			this->_previous = ch;
			return ch;
		}
		template char FileDataSource<char>::advance();
		template unsigned char FileDataSource<unsigned char>::advance();

		template <typename CharT>
		CharT FileDataSource<CharT>::peek() {
			return _ifstream.peek();
		}
		template char FileDataSource<char>::peek();
		template unsigned char FileDataSource<unsigned char>::peek();

		template <typename CharT>
		bool FileDataSource<CharT>::isAtEnd() {
			return peek() == EOF;
		}
		template bool FileDataSource<char>::isAtEnd(); 
		template bool FileDataSource<unsigned char>::isAtEnd(); 
	}
}