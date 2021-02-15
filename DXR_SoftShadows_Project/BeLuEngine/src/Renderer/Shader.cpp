#include "stdafx.h"
#include "Shader.h"
#include "DXILShaderCompiler.h"

Shader::Shader(LPCTSTR path, SHADER_TYPE type)
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

	if (m_Type == SHADER_TYPE::VS)
	{
		entryPoint = L"VS_main";
		shaderModelTarget = L"vs_6_5";
	}
	else if (m_Type == SHADER_TYPE::PS)
	{
		entryPoint = L"PS_main";
		shaderModelTarget = L"ps_6_5";
	}
	else if (m_Type == SHADER_TYPE::CS)
	{
		entryPoint = L"CS_main";
		shaderModelTarget = L"cs_6_5";
	}
	else if (m_Type == SHADER_TYPE::DXR)
	{
		entryPoint = L"";
		shaderModelTarget = L"lib_6_5";
	}

	shaderCompilerDesc.entryPoint = entryPoint.c_str();
	shaderCompilerDesc.targetProfile = shaderModelTarget.c_str();


	DXILShaderCompiler::Get()->CompileFromFile(&shaderCompilerDesc, &m_pBlob);

	if (m_pBlob == nullptr)
	{
		BL_LOG_CRITICAL("blob is nullptr when loading shader with path: %S\n", m_Path);
	}
}
