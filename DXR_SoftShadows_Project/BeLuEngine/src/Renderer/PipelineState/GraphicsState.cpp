#include "stdafx.h"
#include "GraphicsState.h"

#include "../RootSignature.h"
#include "../Shader.h"

GraphicsState::GraphicsState(ID3D12Device5* device, RootSignature* rootSignature, const std::wstring& VSName, const std::wstring& PSName, D3D12_GRAPHICS_PIPELINE_STATE_DESC* gpsd, const std::wstring& psoName)
	:PipelineState(psoName)
{
	// Set the rootSignature in the pipeline state object descriptor
	m_pGPSD = gpsd;

	m_pGPSD->pRootSignature = rootSignature->GetRootSig();

	m_pVS = createShader(VSName, ShaderType::VS);
	m_pPS = createShader(PSName, ShaderType::PS);

	IDxcBlob* vsBlob = m_pVS->GetBlob();
	IDxcBlob* psBlob = m_pPS->GetBlob();

	m_pGPSD->VS.pShaderBytecode = vsBlob->GetBufferPointer();
	m_pGPSD->VS.BytecodeLength = vsBlob->GetBufferSize();
	m_pGPSD->PS.pShaderBytecode = psBlob->GetBufferPointer();
	m_pGPSD->PS.BytecodeLength = psBlob->GetBufferSize();

	// Create pipelineStateObject
	HRESULT hr = device->CreateGraphicsPipelineState(m_pGPSD, IID_PPV_ARGS(&m_pPSO));

	if (FAILED(hr))
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to create %S\n", psoName);
	}
	m_pPSO->SetName(psoName.c_str());
}

GraphicsState::~GraphicsState()
{
}

const D3D12_GRAPHICS_PIPELINE_STATE_DESC* GraphicsState::GetGpsd() const
{
	return m_pGPSD;
}

Shader* GraphicsState::GetShader(ShaderType type) const
{
	if (type == ShaderType::VS)
	{
		return m_pVS;
	}
	else if (type == ShaderType::PS)
	{
		return m_pPS;
	}
	
	Log::PrintSeverity(Log::Severity::CRITICAL, "There is no ComputeShader in \'%S\'\n", m_PsoName);
	return nullptr;
}
