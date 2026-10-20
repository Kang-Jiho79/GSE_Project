#pragma once

#include <string>
#include <vector>

// Writes JSON out as it goes, on one line: objects and arrays opened and
// closed in turn, values with or without a key depending on whether they sit
// in an object or an array. Strings are escaped; UTF-8 passes through as is.
// Enough for the monitor's messages - not a parser, and it does not check
// that what it is given makes sense.
class JsonWriter
{
public:
	// key: the member's name inside an object, or nullptr inside an array (or
	// for the outermost value).
	void BeginObject(const char* key = nullptr);
	void EndObject();
	void BeginArray(const char* key = nullptr);
	void EndArray();

	void Value(const char* key, const char* value);
	void Value(const char* key, const std::string& value);
	void Value(const char* key, int value);
	void Value(const char* key, float value);
	void Value(const char* key, bool value);
	void Null(const char* key);

	const std::string& Text() const;
	void Clear();

private:
	// A comma before anything but the first in its object or array, then
	// the key if there is one.
	void Lead(const char* key);
	void Quote(const char* text);

	std::string m_Text;
	// For each open object or array: whether anything is in it yet.
	std::vector<bool> m_Filled;
};
