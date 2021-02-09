#include "stdafx.h"
#include "PipelineState.h"

#include "../Shader.h"
#include "../RootSignature.h"
#include "../Misc/AssetLoader.h"

PipelineState::PipelineState(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	PSO_STREAM* pso_stream,
	const std::wstring& psoName)
{
	m_PsoName = psoName;

	// Set the rootSignature in the pipeline state object descriptor
	pso_stream->rootSig = rootSignature->GetRootSig();

	m_shaders[ShaderType::VS] = createShader(VSName, ShaderType::VS);
	m_shaders[ShaderType::PS] = createShader(PSName, ShaderType::PS);

	ID3DBlob* vsBlob = m_shaders[ShaderType::VS]->GetBlob();
	ID3DBlob* psBlob = m_shaders[ShaderType::PS]->GetBlob();

	pso_stream->vsShaderByteCode.BytecodeLength  = vsBlob->GetBufferSize();
	pso_stream->vsShaderByteCode.pShaderBytecode = vsBlob->GetBufferPointer();
	pso_stream->psShaderByteCode.BytecodeLength  = psBlob->GetBufferSize();
	pso_stream->psShaderByteCode.pShaderBytecode = psBlob->GetBufferPointer();

	// Create pipelineStateObject
	const D3D12_PIPELINE_STATE_STREAM_DESC psoStreamDesc = { sizeof(PSO_STREAM), pso_stream };

	ID3DX12PipelineParserCallbacks callBacks = {};
	HRESULT hr = D3DX12ParsePipelineStream(psoStreamDesc, &callBacks);

	hr = device->CreatePipelineState(&psoStreamDesc, IID_PPV_ARGS(&m_pPSO));

	if (FAILED(hr))
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to create %S\n", psoName.c_str());
	}
	m_pPSO->SetName(psoName.c_str());
}

PipelineState::~PipelineState()
{
	SAFE_RELEASE(&m_pPSO);
}

ID3D12PipelineState* PipelineState::GetPSO() const
{
	return m_pPSO;
}

const Shader* PipelineState::GetShader(ShaderType type) const
{
	return m_shaders.at(type);
}

Shader* PipelineState::createShader(const std::wstring& fileName, ShaderType type)
{
	if (type == ShaderType::VS)
	{
		return AssetLoader::Get()->loadShader(fileName, type);
	}
	else if (type == ShaderType::PS)
	{
		return AssetLoader::Get()->loadShader(fileName, type);
	}
	else if (type == ShaderType::CS)
	{
		return AssetLoader::Get()->loadShader(fileName, type);
	}

	Log::PrintSeverity(Log::Severity::CRITICAL, "Shader with name '%s' is not supported\n", fileName.c_str());
	return nullptr;
}
