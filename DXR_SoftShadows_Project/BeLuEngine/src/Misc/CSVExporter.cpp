#include "stdafx.h"
#include "CSVExporter.h"

CSVExporter::CSVExporter(int numColumns, std::wstring* columnTitles)
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


	return true;
}

std::stringstream& operator<<(CSVExporter& exporter, std::string string)
{
	exporter.entries << string;

	return exporter.entries;
}
