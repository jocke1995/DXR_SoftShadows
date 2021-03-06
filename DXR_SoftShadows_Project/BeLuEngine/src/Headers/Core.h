#ifndef CORE_H
#define CORE_H

#include <string>
#include <locale>
#include <codecvt>
#include <vector>
#include <Windows.h>
#include <filesystem>

static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> strconverter;
inline std::string to_string(std::wstring wstr)
{
	return strconverter.to_bytes(wstr);
}
inline std::wstring to_wstring(std::string str)
{
	return strconverter.from_bytes(str);
}


template <typename T>
inline T Min(T a, T b)
{
	if (a < b)
	{
		return a;
	}
	return b;
}

template <typename T>
inline T Max(T a, T b)
{
	if (a > b)
	{
		return a;
	}
	return b;
}

inline std::string GetFileExtension(const std::string& FileName)
{
	if (FileName.find_last_of(".") != std::string::npos)
	{
		return FileName.substr(FileName.find_last_of(".") + 1);
	}
	return "";
}

enum class WINDOW_MODE
{
	WINDOWED,
	WINDOWED_FULLSCREEN,
	FULLSCREEN
};

enum class TEXTURE_TYPE
{
	UNKNOWN,
	TEXTURE2D,
	TEXTURE2DGUI,
	TEXTURECUBEMAP,
	NUM_TYPES
};

enum class TEXTURE2D_TYPE
{
	ALBEDO,
	ROUGHNESS,
	METALLIC,
	NORMAL,
	EMISSIVE,
	OPACITY,
	NUM_TYPES
};

enum LIGHT_TYPE
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
	NUM_LIGHT_TYPES
};

enum SHADOW_RESOLUTION
{
	LOW,
	MEDIUM,
	HIGH,
	NUM_SHADOW_RESOLUTIONS,
	UNDEFINED
};

enum class SHADER_TYPE
{
	VS = 0,
	PS = 1,
	CS = 2,
	DXR = 3,
	NUM_SHADER_TYPES
};

enum class CAMERA_TYPE
{
	PERSPECTIVE,
	ORTHOGRAPHIC,
	NUM_CAMERA_TYPES,
	UNDEFINED
};

// this will only call release if an object exists (prevents exceptions calling release on non existant objects)
#define SAFE_RELEASE(p)			\
{								\
	if ((*p))					\
	{							\
		(*p)->Release();		\
		(*p) = nullptr;			\
	}							\
}

// Debug

#define NUM_SWAP_BUFFERS 2
#define BIT(x) (1 << x)
#define MAXNUMBER 10000000.0f

enum FLAG_DRAW
{
	DRAW_OPAQUE		= BIT(1),
	NUM_FLAG_DRAWS
};

enum FLAG_THREAD
{
	RENDER = BIT(1),
	TEST = BIT(2),
	TEST2 = BIT(3),
	// CopyTextures,
	// PrepareNextScene ..
	// etc
	ALL = BIT(4)
	// etc..
};
enum class Severity
{
	INFO,
	WARNING,
	CRITICAL,
	OTHER
};

class Log
{
public:
	template <typename... Args>
	static void PrintSeverity(
		const std::string& fileName,
		const std::string& lineNumber,
		const Severity type,
		const std::string& string,
		const Args&... args)
	{
		std::vector<char> inputBuffer;
		inputBuffer.resize(4096);
		char typeBuffer[100] = {};

		sprintf(inputBuffer.data(), string.c_str(), args...);

		switch (type)
		{
		case Severity::INFO:
			sprintf(typeBuffer, (std::filesystem::path(fileName).filename().string() + ": Line " + lineNumber + ": INFO: ").c_str());
			break;

		case Severity::WARNING:
			sprintf(typeBuffer, (std::filesystem::path(fileName).filename().string() + ": Line " + lineNumber + ": WARNING: ").c_str());
			break;

		case Severity::CRITICAL:
			sprintf(typeBuffer, (std::filesystem::path(fileName).filename().string() + ": Line " + lineNumber + ": CRITICAL ERROR: ").c_str());
			break;

		default:
			sprintf(typeBuffer, "");
			break;
		}

		std::string finalBuffer = std::string(typeBuffer) + inputBuffer.data();

		OutputDebugStringA(finalBuffer.c_str());
	}

	template <typename... Args>
	static void Print(const std::string string, const Args&... args)
	{
		std::vector<char> inputBuffer;
		inputBuffer.resize(4096);

		sprintf(inputBuffer.data(), string.c_str(), args...);

		OutputDebugStringA(inputBuffer.data());
	}
private:
	Log();
};

#ifdef DEBUG
	#define BL_LOG(...)			 Log::Print(__VA_ARGS__)
	#define BL_LOG_INFO(...)	 Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::INFO	  , __VA_ARGS__)
	#define BL_LOG_WARNING(...)	 Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::WARNING , __VA_ARGS__)
	#define BL_LOG_CRITICAL(...) Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::CRITICAL, __VA_ARGS__)
#else
	#define BL_LOG(...)
	#define BL_LOG_INFO(...)
	#define BL_LOG_WARNING(...)
	#define BL_LOG_CRITICAL(...)
#endif

#endif
