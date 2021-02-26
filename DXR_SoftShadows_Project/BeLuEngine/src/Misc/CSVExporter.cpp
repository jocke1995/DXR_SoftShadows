#include "stdafx.h"
#include "CSVExporter.h"

#include <iostream>

CSVExporter::CSVExporter()
{
}

CSVExporter::~CSVExporter()
{
}

void CSVExporter::Clear()
{
	entries.str("");
	entries.clear();
}

bool CSVExporter::Export(const std::wstring& name, const std::wstring& path)
{
	std::ofstream outFile;
	// open file, creates if not exists
	outFile.open(path + name, std::ofstream::out | std::ofstream::trunc);

	if (!outFile.is_open())
	{
		// Something went wrong
		return false;
	}
	
	// Write to file
	outFile << entries.rdbuf();


	outFile.close();

	return true;
}

bool CSVExporter::Append(const std::wstring& name, const std::wstring& path)
{
	std::ofstream outFile;
	// open file, creates if not exists
	outFile.open(path + name, std::ofstream::out | std::ofstream::app);

	if (!outFile.is_open())
	{
		// Something went wrong
		return false;
	}

	// Write to file
	outFile << entries.rdbuf();


	outFile.close();

	return true;
}

bool CSVExporter::IsFileEmpty(const std::wstring& name, const std::wstring& path)
{
	// If file don't exist, it's "empty"
	if (!std::filesystem::exists(path + name))
	{
		return true;
	}

	std::ifstream file;
	file.open(path + name, std::ifstream::in | std::ifstream::trunc);

	if (!file.is_open())
	{
		// Something went wrong
		return false;
	}

	bool r = file.peek() == std::ifstream::traits_type::eof();

	file.close();

	return r;
}

std::stringstream& operator<<(CSVExporter& exporter, std::string string)
{
	exporter.entries << string;

	return exporter.entries;
}

std::stringstream& operator<<(CSVExporter& exporter, double d)
{
	return operator<<(exporter, std::to_string(d));
}

std::stringstream& operator<<(CSVExporter& exporter, float d)
{
	return operator<<(exporter, std::to_string(d));
}

std::stringstream& operator<<(CSVExporter& exporter, int d)
{
	return operator<<(exporter, std::to_string(d));
}

std::stringstream& operator<<(CSVExporter& exporter, bool d)
{
	std::string boolStr = "False";
	if (d)
	{
		boolStr = "True";
	}
	return operator<<(exporter, boolStr);
}
