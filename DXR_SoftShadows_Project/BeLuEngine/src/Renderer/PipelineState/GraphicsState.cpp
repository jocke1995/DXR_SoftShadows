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

	m_pVS = createShader(VSName, SHADER_TYPE::VS);
	m_pPS = createShader(PSName, SHADER_TYPE::PS);

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
		BL_LOG_CRITICAL("Failed to create %S\n", psoName);
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

Shader* GraphicsState::GetShader(SHADER_TYPE type) const
{
	if (type == SHADER_TYPE::VS)
	{
		return m_pVS;
	}
	else if (type == SHADER_TYPE::PS)
	{
		return m_pPS;
	}
	
	BL_LOG_CRITICAL("There is no ComputeShader in \'%S\'\n", m_PsoName);
	return nullptr;
}
