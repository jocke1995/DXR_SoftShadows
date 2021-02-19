#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include <sstream>

class CSVExporter
{
public:
	CSVExporter();
	~CSVExporter();

	void Clear();

	bool Export(const std::wstring& name = L"Results.csv", const std::wstring& path = L"./");
	bool Append(const std::wstring& name = L"Results.csv", const std::wstring& path = L"./");
	// Returns true if file is empty (even if it does not exist)
	bool IsFileEmpty(const std::wstring& name = L"Results.csv", const std::wstring& path = L"./");

	inline void Print()
	{
		std::string a = entries.str();  
		Log::Print("%s\n", a.c_str());
	}

	friend std::stringstream& operator<<(CSVExporter& exporter, bool d);
	friend std::stringstream& operator<<(CSVExporter& exporter, std::string string);
	friend std::stringstream& operator<<(CSVExporter& exporter, double d);
	friend std::stringstream& operator<<(CSVExporter& exporter, float d);
	friend std::stringstream& operator<<(CSVExporter& exporter, int d);
	
	//friend std::stringstream& operator<<(CSVExporter& exporter, std::wstring wstring); does not work for some reason.

private:
	std::stringstream entries;   // Holds all data to be output | csv format (asd:123:qwe)
};



#endif // !CSVEXPORTER_H

