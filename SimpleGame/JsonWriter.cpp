#include "stdafx.h"
#include "JsonWriter.h"
#include <cstdio>

void JsonWriter::Lead(const char* key)
{
	if (!m_Filled.empty())
	{
		if (m_Filled.back())
		{
			m_Text += ',';
		}
		m_Filled.back() = true;
	}
	if (key != nullptr)
	{
		Quote(key);
		m_Text += ':';
	}
}

void JsonWriter::Quote(const char* text)
{
	m_Text += '"';
	for (const char* c = text; *c != '\0'; c++)
	{
		unsigned char ch = (unsigned char)*c;
		switch (ch)
		{
		case '"':
		{
			m_Text += "\\\"";
			break;
		}
		case '\\':
		{
			m_Text += "\\\\";
			break;
		}
		case '\n':
		{
			m_Text += "\\n";
			break;
		}
		case '\r':
		{
			m_Text += "\\r";
			break;
		}
		case '\t':
		{
			m_Text += "\\t";
			break;
		}
		default:
		{
			if (ch < 0x20)
			{
				char escaped[8];
				snprintf(escaped, sizeof(escaped), "\\u%04x", ch);
				m_Text += escaped;
			}
			else
			{
				m_Text += (char)ch;
			}
			break;
		}
		}
	}
	m_Text += '"';
}

void JsonWriter::BeginObject(const char* key)
{
	Lead(key);
	m_Text += '{';
	m_Filled.push_back(false);
}

void JsonWriter::EndObject()
{
	m_Text += '}';
	m_Filled.pop_back();
}

void JsonWriter::BeginArray(const char* key)
{
	Lead(key);
	m_Text += '[';
	m_Filled.push_back(false);
}

void JsonWriter::EndArray()
{
	m_Text += ']';
	m_Filled.pop_back();
}

void JsonWriter::Value(const char* key, const char* value)
{
	Lead(key);
	if (value == nullptr)
	{
		m_Text += "null";
		return;
	}
	Quote(value);
}

void JsonWriter::Value(const char* key, const std::string& value)
{
	Value(key, value.c_str());
}

void JsonWriter::Value(const char* key, int value)
{
	Lead(key);
	m_Text += std::to_string(value);
}

void JsonWriter::Value(const char* key, float value)
{
	Lead(key);
	char number[32];
	snprintf(number, sizeof(number), "%.2f", value);
	m_Text += number;
}

void JsonWriter::Value(const char* key, bool value)
{
	Lead(key);
	m_Text += value ? "true" : "false";
}

void JsonWriter::Null(const char* key)
{
	Lead(key);
	m_Text += "null";
}

const std::string& JsonWriter::Text() const
{
	return m_Text;
}

void JsonWriter::Clear()
{
	m_Text.clear();
	m_Filled.clear();
}
