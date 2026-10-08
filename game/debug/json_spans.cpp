#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/json_spans.h"
#include "rapidjson/error/en.h"
#include "rapidjson/reader.h"
#include <set>
#include <stdexcept>

namespace
{
	// Records leaf spans while rapidjson reads. Every callback runs with the
	// stream just past the token it reports.
	class SpanHandler
	{
	public:
		SpanHandler(const rapidjson::StringStream& stream, std::vector<JsonLeaf>& leaves,
			std::size_t& strings) :
			stream_(stream), leaves_(leaves), strings_(strings)
		{
		}

		bool Null() { this->scalar(false); return true; }
		bool Bool(bool value) { this->leaf(JsonLeaf::Kind::boolean, value ? 4u : 5u); return true; }
		bool Int(int) { this->number(); return true; }
		bool Uint(unsigned) { this->number(); return true; }
		bool Int64(std::int64_t) { this->number(); return true; }
		bool Uint64(std::uint64_t) { this->number(); return true; }
		bool Double(double) { this->number(); return true; }
		bool RawNumber(const char*, rapidjson::SizeType, bool) { this->number(); return true; }
		bool String(const char*, rapidjson::SizeType, bool)
		{
			this->scalar(true);

			return true;
		}

		bool StartObject() { this->open(false); return true; }
		bool Key(const char* key, rapidjson::SizeType length, bool)
		{
			this->key_.assign(key, length);
			return true;
		}
		bool EndObject(rapidjson::SizeType) { this->close(); return true; }
		bool StartArray() { this->open(true); return true; }
		bool EndArray(rapidjson::SizeType) { this->close(); return true; }

	private:
		struct Frame
		{
			bool array = false;
			std::size_t index = 0;
			std::string path;
		};

		const rapidjson::StringStream& stream_;
		std::vector<JsonLeaf>& leaves_;
		std::size_t& strings_;
		std::vector<Frame> frames_;
		std::string key_;
		std::set<std::string> seen_;
		// Frames at or below this depth belong to a repeated key: skipped.
		std::size_t suppressed_from_ = 0;

		bool suppressed() const
		{
			return this->suppressed_from_ != 0 && this->frames_.size() >= this->suppressed_from_;
		}

		std::string value_path() const
		{
			if (this->frames_.empty())
			{
				return {};
			}
			const Frame& top = this->frames_.back();
			if (top.array)
			{
				return top.path + "[" + std::to_string(top.index) + "]";
			}
			return top.path.empty() ? this->key_ : top.path + "." + this->key_;
		}

		// After any value inside an array, the next element is one further on.
		void advance()
		{
			if (!this->frames_.empty() && this->frames_.back().array)
			{
				++this->frames_.back().index;
			}
		}

		void open(bool array)
		{
			const std::string path = this->value_path();
			const bool repeated = !this->suppressed() && !this->frames_.empty() &&
				!this->seen_.insert(path).second;
			this->frames_.push_back({ array, 0, path });
			if (repeated)
			{
				this->suppressed_from_ = this->frames_.size();
			}
		}

		void close()
		{
			if (this->suppressed_from_ == this->frames_.size())
			{
				this->suppressed_from_ = 0;
			}
			this->frames_.pop_back();
			this->advance();
		}

		void scalar(bool string)
		{
			if (string && !this->suppressed())
			{
				++this->strings_;
			}
			this->advance();
		}

		static bool number_char(char c)
		{
			return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
		}

		// rapidjson parses a number through a local copy of the stream and
		// writes the position back only after the handler returns, so here
		// the stream may still be at the number's first character. Either
		// way the token is the run of number characters touching it.
		void number()
		{
			const char* text = this->stream_.head_;
			const std::size_t at = this->stream_.Tell();
			std::size_t start = at;
			std::size_t end = at;
			if (number_char(text[at]) && (at == 0 || !number_char(text[at - 1])))
			{
				while (number_char(text[end]))
				{
					++end;
				}
			}
			else
			{
				while (start > 0 && number_char(text[start - 1]))
				{
					--start;
				}
			}
			this->record(JsonLeaf::Kind::number, start, end);
		}

		void leaf(JsonLeaf::Kind kind, std::size_t length)
		{
			const std::size_t end = this->stream_.Tell();
			this->record(kind, end - length, end);
		}

		void record(JsonLeaf::Kind kind, std::size_t begin, std::size_t end)
		{
			const std::string path = this->value_path();
			if (!this->suppressed() && this->seen_.insert(path).second)
			{
				this->leaves_.push_back({ path, kind, begin, end });
			}
			this->advance();
		}
	};

	std::string position(const std::string& text, std::size_t offset)
	{
		std::size_t line = 1;
		std::size_t column = 1;
		for (std::size_t i = 0; i < offset && i < text.size(); ++i)
		{
			if (text[i] == '\n')
			{
				++line;
				column = 1;
			}
			else
			{
				++column;
			}
		}
		return "line " + std::to_string(line) + ", column " + std::to_string(column);
	}
}

JsonSpanDocument::JsonSpanDocument(std::string text, std::string name) :
	text_(std::move(text)), name_(std::move(name))
{
	if (this->text_.find('\0') != std::string::npos)
	{
		throw std::runtime_error(this->name_ + ": contains a NUL byte");
	}
	rapidjson::StringStream stream(this->text_.c_str());
	SpanHandler handler(stream, this->leaves_, this->string_count_);
	rapidjson::Reader reader;
	const rapidjson::ParseResult result =
		reader.Parse<rapidjson::kParseDefaultFlags>(stream, handler);
	if (result.IsError())
	{
		throw std::runtime_error(this->name_ + ": " + position(this->text_, result.Offset()) +
			": " + rapidjson::GetParseError_En(result.Code()));
	}
}

const JsonLeaf* JsonSpanDocument::find(std::string_view path) const
{
	for (const JsonLeaf& leaf : this->leaves_)
	{
		if (leaf.path == path)
		{
			return &leaf;
		}
	}
	return nullptr;
}

std::string_view JsonSpanDocument::token(const JsonLeaf& leaf) const
{
	return std::string_view(this->text_).substr(leaf.begin, leaf.end - leaf.begin);
}

std::string JsonSpanDocument::replaced(const JsonLeaf& leaf, std::string_view token) const
{
	std::string result;
	result.reserve(this->text_.size() + token.size());
	result.append(this->text_, 0, leaf.begin);
	result.append(token);
	result.append(this->text_, leaf.end, std::string::npos);
	return result;
}
#endif
