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
		template <typename CharT>
		DataSource<CharT>::stringType DataSource<CharT>::advanceN(size_t n) {
			stringType result{};
			for (size_t i = 0; i < n; ++i) {
				result += advance();
			}
			return result;
		}
		template std::basic_string<char> DataSource<char>::advanceN(size_t n); 
		StringDataSource::StringDataSource(const std::string& input) : _input(input) {}
		char StringDataSource::advance() {
			if (_index >= _input.size()) {
				return EOF;
			}
			_previous = _input[_index++];
			return _previous;
		}
		char StringDataSource::peek() {
			if (_index >= _input.size()) {
				return EOF;
			}
			return _input[_index];
		}
		bool StringDataSource::isAtEnd() {
			return _index >= _input.size();
		}
		FileDataSource::FileDataSource(const std::string& filename) : _ifstream(filename) {
			if (!_ifstream.is_open()) {
				_isAtEnd = true;
			}
		}
		FileDataSource::~FileDataSource() {
			if (_ifstream.is_open()) {
				_ifstream.close();
			}
		}
		char FileDataSource::advance() {
			char ch;
			if (!_ifstream.get(ch)) {
				_isAtEnd = true;
				return EOF;
			}
			_previous = ch;
			return ch;
		}
		char FileDataSource::peek() {
			return _ifstream.peek();
		}
		bool FileDataSource::isAtEnd() {
			return peek() == EOF;
		}
	}
}