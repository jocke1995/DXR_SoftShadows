#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include <sstream>

class CSVExporter
{
public:
	CSVExporter(int numColumns, std::wstring* columnTitles);
	CSVExporter() {}
	~CSVExporter();

	void Clear();

	bool Export(const std::wstring& name, const std::wstring& path);

	inline void Print() {
		Log::Print(entries.str() + '\n');
	}

	friend std::stringstream& operator<<(CSVExporter& exporter, std::string string);

private:
	std::stringstream entries;   // Holds all data to be output, std::wstring is csv format (asd:123:qwe)
};



#endif // !CSVEXPORTER_H

