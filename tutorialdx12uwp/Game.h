//
// Game.h
//

#pragma once

#include "StepTimer.h"
#include "Mesh.h"
#include "HelperFunctions.h"
#include "DDSTextureLoader.h"


// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game
{
public:

    Game() noexcept ;

    // Initialization and management
    void Initialize(::IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
    void ValidateDevice();

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
    void LoadMeshes();

private:

    size_t m_NumberOfMeshes;

    // Per object particular information
    struct ObjectData {
        XMFLOAT4X4											matrixWorld;
        UINT                                                matind;
     };
   
    // One shape each time
    std::vector<std::vector<ObjectData>> m_objects; // Each element is per shape information. Each per shape information is per instance data.
    
    std::vector<std::shared_ptr<Mesh>>					m_meshes; // One mesh per shape


    XMFLOAT4X4											m_view;
    XMFLOAT4X4											m_projection;


    void CreateMainInputFlowResources(const std::vector<std::shared_ptr<Mesh>>& mesh);
    

    // Vertex and index related stuff
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_vBufferDefault; // Buffer para vértices
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_vBufferUpload; // Buffer para vértices
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_iBufferDefault; // Buffer para indices
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_iBufferUpload; // Buffer para indices
    D3D12_VERTEX_BUFFER_VIEW				m_vBufferView;
    D3D12_INDEX_BUFFER_VIEW				m_iBufferView;
    
    // Textures related stuff

    
    
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>  m_textureDefault;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>  m_textureUpload;


    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_rootSignature;
    
    
   
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		m_cDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_sDescriptorHeap; // Descriptor HEap de Samplers
    unsigned int m_cDescriptorSize;

    struct vConstants {

        DirectX::XMFLOAT4X4 PassTransform = { 1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0 };
       

    };

    struct vInstance {

        DirectX::XMFLOAT4X4 Transform = { 1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0 };
        DirectX::XMFLOAT4X4 NormalTransform = { 1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0 };
        UINT MaterialIndex;
        UINT Pad0;
        UINT Pad1;
        UINT Pad2;

    };

    

    void LoadPrecompiledShaders();

    Microsoft::WRL::ComPtr<ID3DBlob>					m_vsByteCode;
    Microsoft::WRL::ComPtr<ID3DBlob>					m_psByteCode;
    D3D12_SHADER_BYTECODE								m_vs;
    D3D12_SHADER_BYTECODE								m_ps;

    void PSO();

    std::vector<D3D12_INPUT_ELEMENT_DESC>				m_inputLayout;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC		m_psoDescriptor;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_pso;

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void WaitForGpu();
    void MoveToNextFrame();
    void GetAdapter(IDXGIAdapter1** ppAdapter);

    void OnDeviceLost();

    

    // Application state
    IUnknown*                                           m_window;
    int                                                 m_outputWidth;
    int                                                 m_outputHeight;
    DXGI_MODE_ROTATION                                  m_outputRotation;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                                   m_featureLevel;
    static const UINT                                   c_swapBufferCount = 3;
    UINT                                                m_backBufferIndex;
    UINT                                                m_rtvDescriptorSize;
    Microsoft::WRL::ComPtr<ID3D12Device>                m_d3dDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory4>               m_dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource>				m_vConstantBuffer[c_swapBufferCount]; // Buffer de constantes
    Microsoft::WRL::ComPtr<ID3D12Resource>				m_vInstanceBuffer[c_swapBufferCount]; // Buffer de constantes
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
    UINT64                                              m_fenceValues[c_swapBufferCount];
    vConstants                                          m_vConstants[c_swapBufferCount];
    vInstance                                           m_vInstances[c_swapBufferCount];
    Microsoft::WRL::Wrappers::Event                     m_fenceEvent;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;

    // Game state
    DX::StepTimer                                       m_timer;
};

namespace GameStatics {

    const UINT MaxNumberOfMeshes = 10;

    enum class ShapeName { SHAPE1=0 , SHAPE2=1, SHAPE3=2, SHAPE4=3, SHAPE5=4 };
    static std::map<ShapeName,std::string>  ObjFileNames = {
        {ShapeName::SHAPE1,"Assets/mesh1.obj"},
        {ShapeName::SHAPE2,"Assets/mesh2.obj"},
        {ShapeName::SHAPE3,"Assets/mesh3.obj"},
        {ShapeName::SHAPE4,"Assets/mesh4.obj"},
        {ShapeName::SHAPE5,"Assets/mesh5.obj"}
    };
    
    enum class TexName { TEX1=0,TEX2=1,TEX3=2,TEX4=3,TEX5=4};
    static std::map<TexName, std::wstring> TexFileNames = {
        {TexName::TEX1,L"Assets/tex1.dds"},
        {TexName::TEX2,L"Assets/tex2.dds"},
        {TexName::TEX3,L"Assets/tex3.dds"},
        {TexName::TEX4,L"Assets/tex4.dds"},
        {TexName::TEX5,L"Assets/tex5.dds"}
    };

}