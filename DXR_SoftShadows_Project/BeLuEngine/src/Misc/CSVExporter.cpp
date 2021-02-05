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
	outFile.open(path + name, std::ofstream::out, std::ofstream::trunc);

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

std::stringstream& operator<<(CSVExporter& exporter, std::string string)
{
	exporter.entries << string;

	return exporter.entries;
}