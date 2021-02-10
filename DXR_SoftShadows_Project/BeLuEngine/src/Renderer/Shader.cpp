#include "stdafx.h"
#include "Shader.h"
#include "DXILShaderCompiler.h"

Shader::Shader(LPCTSTR path, ShaderType type)
{
	m_Path = path;
	m_Type = type;

	compileShader();
}

Shader::~Shader()
{
	SAFE_RELEASE(&m_pBlob);
}

IDxcBlob* Shader::GetBlob() const
{
	return m_pBlob;
}

void Shader::compileShader()
{
	DXILShaderCompiler::DXILCompilationDesc shaderCompilerDesc = {};

	shaderCompilerDesc.compileArguments.push_back(L"/Gis"); // ? floating point accuracy?
#ifdef _DEBUG
	shaderCompilerDesc.compileArguments.push_back(L"/Zi"); // Debug info
	shaderCompilerDesc.defines.push_back({ L"_DEBUG" });
#endif

	shaderCompilerDesc.filePath = m_Path;
	std::wstring entryPoint;
	std::wstring shaderModelTarget;

	if (m_Type == ShaderType::VS)
	{
		entryPoint = L"VS_main";
		shaderModelTarget = L"vs_6_5";
	}
	else if (m_Type == ShaderType::PS)
	{
		entryPoint = L"PS_main";
		shaderModelTarget = L"ps_6_5";
	}
	else if (m_Type == ShaderType::CS)
	{
		entryPoint = L"CS_main";
		shaderModelTarget = L"cs_6_5";
	}

	shaderCompilerDesc.entryPoint = entryPoint.c_str();
	shaderCompilerDesc.targetProfile = shaderModelTarget.c_str();


	DXILShaderCompiler::Get()->CompileFromFile(&shaderCompilerDesc, &m_pBlob);

	//	unsigned int flags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
	//#if defined( DEBUG ) || defined( _DEBUG )
	//	flags |= D3DCOMPILE_DEBUG;
	//#endif
	//
	//	HRESULT hr = D3DCompileFromFile(
	//		m_Path, // filePath + filename
	//		nullptr,		// optional macros
	//		D3D_COMPILE_STANDARD_FILE_INCLUDE,		// optional include files
	//		entryPoint.c_str(),		// entry point
	//		shaderModelTarget.c_str(),		// shader model (target)
	//		flags,	// shader compile options			// here DEBUGGING OPTIONS
	//		0,				// effect compile options
	//		&m_pBlob,	// double pointer to ID3DBlob		
	//		&errorMessages			// pointer for Error Blob messages.
	//						// how to use the Error blob, see here
	//						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	//	);


	if (m_pBlob == nullptr)
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "blob is nullptr when loading shader with path: %S\n", m_Path);
	}

	//if (FAILED(hr) && errorMessages)
	//{
	//	const char* errorMsg = (const char*)errorMessages->GetBufferPointer();
	//
	//	Log::PrintSeverity(Log::Severity::CRITICAL, "%s\n", errorMsg);
	//}
}
