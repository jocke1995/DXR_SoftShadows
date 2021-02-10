#ifndef DXILSHADERCOMPILER_H
#define DXILSHADERCOMPILER_H

#include <Windows.h>
#include <vector>
#include <dxcapi.h>

class DXILShaderCompiler
{
public:
	~DXILShaderCompiler();
	static DXILShaderCompiler* Get();

	struct DXILCompilationDesc
	{
		LPCVOID source = nullptr;
		LPCWSTR filePath;
		LPCWSTR entryPoint;
		LPCWSTR targetProfile;
		std::vector<LPCWSTR> compileArguments;
		std::vector<DxcDefine> defines;
	};

	HRESULT Init();

	// Compiles a shader into a blob
	// Compiles from source if source != nullptr, otherwise from file
	HRESULT CompileFromFile(DXILCompilationDesc* desc, IDxcBlob** ppResult);

private:
	DXILShaderCompiler();
	

	IDxcLibrary* m_library = nullptr;
	IDxcCompiler* m_compiler = nullptr;
	IDxcLinker* m_linker = nullptr;
	IDxcIncludeHandler* m_includeHandler = nullptr;
};

#endif