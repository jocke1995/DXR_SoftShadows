#ifndef PIPELINESTATE_H
#define PIPELINESTATE_H

class Shader;
class RootSignature;
class AssetLoader;

// DX12 Forward Declarations
struct ID3D12PipelineState;

struct PSO_STREAM
{
    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
    ID3D12RootSignature* rootSig = nullptr;

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
    D3D12_SHADER_BYTECODE vsShaderByteCode = {};

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
    D3D12_SHADER_BYTECODE psShaderByteCode = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type3 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS;
    //D3D12_SHADER_BYTECODE dsShaderByteCode;
    //
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type4 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS;
    //D3D12_SHADER_BYTECODE hsShaderByteCode;
    //
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type5 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS;
    //D3D12_SHADER_BYTECODE gsShaderByteCode;
    //
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type6 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
    //D3D12_SHADER_BYTECODE csShaderByteCode;
    //
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type7 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT;
    //D3D12_STREAM_OUTPUT_DESC stream_output_desc;

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type8 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
    D3D12_BLEND_DESC blend_desc = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type9 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK;
    //UINT sample_mask = {};

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type10 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
    D3D12_RASTERIZER_DESC rasterizer_desc = {};

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type11 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
    D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = {};
    
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type12 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
    //D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type13 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE;
    //D3D12_INDEX_BUFFER_STRIP_CUT_VALUE index_buffer_strip_cut_value = {};

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type14 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type = {};

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type15 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
    D3D12_RT_FORMAT_ARRAY render_target_info = {};

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type16 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
    DXGI_FORMAT depth_stencil_format = {};

    D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type17= D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
    DXGI_SAMPLE_DESC sample_desc = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type18 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK;
    //D3D12_NODE_MASK node_mask = {};
    
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type19 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO;
    //D3D12_CACHED_PIPELINE_STATE cached_pso = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type20 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS;
    //D3D12_PIPELINE_STATE_FLAGS pipeline_state_flags = {};

    // Todo: fel p� msdn? 
    // source: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_pipeline_state_subobject_type
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type21 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1;
    //D3D12_DEPTH_STENCIL_DESC1 depth_stencil_desc1 = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type22 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING;
    //D3D12_VIEW_INSTANCING_DESC view_instancing_desc = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type23 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS;
    //D3D12_SHADER_BYTECODE asShaderByteCode = {};
    //
    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type24 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS;
    //D3D12_SHADER_BYTECODE msShaderByteCode = {};

    //D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type25 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MAX_VALID;
};

// 1. Skit i de nya grafiks/compute staten, tror inte vi beh�ver de �nd�
// 2. f�rs�k l�sa

// har inget g�ra med raytracing PSO

class PipelineState
{
public:
	PipelineState(
		ID3D12Device5* device,
		RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
        PSO_STREAM* pso_stream,
		const std::wstring& psoName);
	virtual ~PipelineState();

	ID3D12PipelineState* GetPSO() const;

	const Shader* GetShader(ShaderType type) const;

protected:
	ID3D12PipelineState* m_pPSO = nullptr;
	std::wstring m_PsoName = L"DefaultName";

	std::map<ShaderType, Shader*> m_shaders;
	Shader* createShader(const std::wstring& fileName, ShaderType type);
};

#endif