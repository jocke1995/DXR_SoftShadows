#include "stdafx.h"
#include "../Headers/Core.h"
#include "DXILShaderCompiler.h"

DXILShaderCompiler::DXILShaderCompiler()
{
}

DXILShaderCompiler::~DXILShaderCompiler() 
{
	SAFE_RELEASE(&m_compiler);
	SAFE_RELEASE(&m_library);
	SAFE_RELEASE(&m_includeHandler);
	SAFE_RELEASE(&m_linker);
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

	if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler))))
	{
		if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library))))
		{
			if (SUCCEEDED(m_library->CreateIncludeHandler(&m_includeHandler)))
			{
				if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&m_linker))))
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
		hr = m_library->CreateBlobFromFile(desc->filePath, nullptr, &source);

		if (SUCCEEDED(hr))
		{
			IDxcOperationResult* pResult = nullptr;
			if (SUCCEEDED(hr = m_compiler->Compile(
				source,									// program text
				desc->filePath,							// file name, mostly for error messages
				desc->entryPoint,						// entry point function
				desc->targetProfile,					// target profile
				desc->compileArguments.data(),          // compilation arguments
				(UINT)desc->compileArguments.size(),	// number of compilation arguments
				desc->defines.data(),					// define arguments
				(UINT)desc->defines.size(),				// number of define arguments
				m_includeHandler,						// handler for #include directives
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
							m_library->GetBlobAsUtf16(pPrintBlob, &pPrintBlob16);

							OutputDebugStringW((LPCWSTR)pPrintBlob16->GetBufferPointer());
							MessageBox(0, (LPCWSTR)pPrintBlob16->GetBufferPointer(), L"", 0);


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
