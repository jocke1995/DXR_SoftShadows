#include "stdafx.h"
#include "../Headers/Core.h"
#include "DXILShaderCompiler.h"

DXILShaderCompiler::DXILShaderCompiler()
{
}

DXILShaderCompiler::~DXILShaderCompiler() 
{
	SAFE_RELEASE(&m_pCompiler);
	SAFE_RELEASE(&m_pLibrary);
	SAFE_RELEASE(&m_pIncludeHandler);
	SAFE_RELEASE(&m_pLinker);
}

DXILShaderCompiler* DXILShaderCompiler::Get()
{
	static DXILShaderCompiler instance;
	
	return &instance;
}

HRESULT DXILShaderCompiler::Init() 
{

	HMODULE dll = LoadLibraryA("dxcompiler.dll");
	if (dll == false)
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "dxcompiler.dll is missing");
	}

	DxcCreateInstanceProc pfnDxcCreateInstance = DxcCreateInstanceProc(GetProcAddress(dll, "DxcCreateInstance"));

	HRESULT hr = E_FAIL;

	if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler))))
	{
		if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_pLibrary))))
		{
			if (SUCCEEDED(m_pLibrary->CreateIncludeHandler(&m_pIncludeHandler)))
			{
				if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&m_pLinker))))
				{
					Log::Print("DXIL Compiler succesfully created!\n");
				}
			}
		}
	}

	return hr;
}

HRESULT DXILShaderCompiler::CompileFromFile(DXILCompilationDesc* desc, IDxcBlob** ppResult) {
	HRESULT hr = E_FAIL;

	if (desc != nullptr)
	{
		IDxcBlobEncoding* source = nullptr;

		// Compile from file path
		hr = m_pLibrary->CreateBlobFromFile(desc->filePath, nullptr, &source);

		if (SUCCEEDED(hr))
		{
			IDxcOperationResult* pResult = nullptr;
			if (SUCCEEDED(hr = m_pCompiler->Compile(
				source,									// program text
				desc->filePath,							// file name, mostly for error messages
				desc->entryPoint,						// entry point function
				desc->targetProfile,					// target profile
				desc->compileArguments.data(),          // compilation arguments
				(UINT)desc->compileArguments.size(),	// number of compilation arguments
				desc->defines.data(),					// define arguments
				(UINT)desc->defines.size(),				// number of define arguments
				m_pIncludeHandler,						// handler for #include directives
				&pResult))) 
			{
				HRESULT hrCompile = E_FAIL;
				if (SUCCEEDED(hr = pResult->GetStatus(&hrCompile)))
				{
					if (SUCCEEDED(hrCompile))
					{
						if (ppResult)
						{
							pResult->GetResult(ppResult);
							hr = S_OK;
						} 
						else 
						{
							hr = E_FAIL;
						}
					} 
					else 
					{
						IDxcBlobEncoding* pPrintBlob = nullptr;
						if (SUCCEEDED(pResult->GetErrorBuffer(&pPrintBlob)))
						{
							// We can use the library to get our preferred encoding.
							IDxcBlobEncoding* pPrintBlob16 = nullptr;
							m_pLibrary->GetBlobAsUtf16(pPrintBlob, &pPrintBlob16);



							std::wstring b = (LPCWSTR)pPrintBlob16->GetBufferPointer();
							std::string a = to_string(b);
							Log::PrintSeverity(Log::Severity::CRITICAL, "%s\n", a.c_str());


							SAFE_RELEASE(&pPrintBlob);
							SAFE_RELEASE(&pPrintBlob16);
						}
					}
					SAFE_RELEASE(&pResult);
				}
			}
		}
	}
	return hr;
}
