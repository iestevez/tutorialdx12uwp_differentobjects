//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "GameGeo.h"

extern void ExitGame();

using namespace DirectX;

using Microsoft::WRL::ComPtr;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::Graphics::Display;

Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(800),
    m_outputHeight(600),
    m_outputRotation(DXGI_MODE_ROTATION_IDENTITY),
    m_featureLevel(D3D_FEATURE_LEVEL_11_0),
    m_backBufferIndex(0),
    m_rtvDescriptorSize(0),
    m_fenceValues{}
{
}

void Game::LoadMeshes() {
    m_NumberOfMeshes = GameStatics::ObjFileNames.size();
    assert(m_NumberOfMeshes <= c_NumberOfObjects);
    m_meshes.resize(m_NumberOfMeshes);
    m_objects.resize(m_NumberOfMeshes);

    for (auto it = GameStatics::ObjFileNames.begin(); it != GameStatics::ObjFileNames.end();it++) {
        const std::string fileName = it->second;
        //std::shared_ptr<Mesh> ptrMesh = std::make_shared<Mesh>(); //Mesh constructor with std::string parameter tries to load the file
        std::shared_ptr<Mesh> ptrMesh = std::make_shared<Mesh>(fileName); //Mesh constructor with std::string parameter tries to load the file
        m_meshes[static_cast<unsigned int>(it->first)]=ptrMesh;
     }
}


void Game::InitializeObjects(const std::vector<int> &ninstances, const std::vector<int> &matIndex, const float r, const float minDistance, const float maxDistance) {
    assert(m_NumberOfMeshes <= c_NumberOfObjects);
    
    if (matIndex.size() != ninstances.size())
        return;
    m_objects.clear();
    m_objects.resize(m_NumberOfMeshes);
    
    XMMATRIX projection =XMLoadFloat4x4(&m_projection);
    XMMATRIX view = XMLoadFloat4x4(&m_view);
    for (int i = 0; i < m_NumberOfMeshes; i++) {
        int ninst;
        if (i < size(ninstances))
            ninst = ninstances[i];
        else
            ninst = 0;
        
        assert(ninst <= c_NumberOfInstancesPerObject);
        
        m_objects[i].clear();
        m_objects[i].resize(ninst);
        for (int j = 0; j < ninst;j++) {
            
            ObjectData &objectData=m_objects[i][j];
            objectData.isInstanced = true;
            objectData.matind = matIndex[i];

            XMMATRIX tMat = Geo::GetRandomPointInsideFrustum(projection, view, r, minDistance, maxDistance);
            XMMATRIX rMat = Geo::GetRandomRotationMatrix();
            XMMATRIX world = rMat * tMat;
            XMStoreFloat4x4(&objectData.matrixWorld,world);
            
        }

    }

}
void Game::Initialize(::IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation)
{
    // Load Assets
    LoadMeshes();
   
    // Initialize Controller
    m_controller = std::make_shared<Controller>(CoreWindow::GetForCurrentThread());
    

    // Inicializamos matrices de transformación
    float x = 0.0;
    float y = 0.0;
    float z = -10;

    // Recalculamos la matriz de vista
    XMVECTOR location = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorSet(0.0, 0.0, 1.0, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(location, target, up);
    float r = static_cast<float>(m_outputWidth / m_outputHeight);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(0.25 * XM_PI, r, 0.5f, 1000.0f);
    XMStoreFloat4x4(&m_view, view);
   
    XMStoreFloat4x4(&m_projection, projection);



    // Inicializamos un par de objectos
    std::vector<int> ninstances = { 2,3,3 };
    std::vector<int> materials = { 1,1,1 };
    float minDistance = 10.0;
    float maxDistance = 100.0;
    InitializeObjects(ninstances, materials, r, minDistance,maxDistance);

    
    // Windows
    m_window = window; // Inicializamos a la ventana de visualización.
    m_outputWidth = std::max(width, 1); // Ancho y alto
    m_outputHeight = std::max(height, 1);
    m_outputRotation = rotation; // Rotación.

    CreateDevice(); // Creamos el dispotivo
    CreateResources(); // Creamos recursos que dependen del tamaño de la ventana de visualización

    CreateMainInputFlowResources(m_meshes); //Creamos recursos y objetos D3D12 que permiten el flujo de entrada de datos al pipeline
    LoadPrecompiledShaders(); // Cargamos shaders precompilados

    PSO(); // Creamos un estado del pipeline básico.

   


}



// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}


void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Actualización de las transformaciones en la escena

    //float x = 0.0;
    //float y = 0.0;
    //float z = -10;

    m_controller->Update();

    XMVECTOR velocity;
    XMFLOAT3 fVelocity = m_controller->Velocity();
    velocity = XMLoadFloat3(&fVelocity);
    m_Position += velocity * elapsedTime;
    XMFLOAT3 fLookDirection = m_controller->LookDirection();
    m_LookDirection = XMLoadFloat3(&fLookDirection);
    XMMATRIX view = UpdateView();


    // Recalculamos la matriz de vista
    //XMVECTOR location = XMVectorSet(x, y, z, 1.0f);
    //XMVECTOR target = XMVectorSet(0.0, 0.0, 1.0, 1.0f);
    //XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    //XMMATRIX view = XMMatrixLookAtLH(location, target, up);
    float r = static_cast<float>(m_outputWidth / m_outputHeight);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(0.25 * XM_PI, r, 0.5f, 1000.0f);
    XMStoreFloat4x4(&m_view, view);

    static float delta = 0.0;
    delta += static_cast<float>(elapsedTime * (0.1 * XM_2PI));
    //void* data=nullptr;

    // Update data to be uploaded:

    // First, update of pass constants
    XMFLOAT4X4 passTransform;
    XMStoreFloat4x4(&passTransform, XMMatrixIdentity());

    // Second, update of per object constants
    m_vInstances[m_backBufferIndex].clear();
    m_vInstances[m_backBufferIndex].resize(m_objects.size());

    for (int i = 0; i < m_objects.size();i++) {
        std::vector objInstances = m_objects[i];
        m_vInstances[m_backBufferIndex][i].clear();
        m_vInstances[m_backBufferIndex][i].resize(objInstances.size());
        int count = 0;
        for (auto &obj : objInstances) {
            XMMATRIX world = XMLoadFloat4x4(&obj.matrixWorld);
            XMMATRIX rotation = XMMatrixRotationX(delta);
            XMMATRIX translation = XMMatrixTranslation(0.0, 0.0, 0.0);
            world = rotation * translation*world;


            XMMATRIX worldview = world * view;
            XMMATRIX transform = worldview * projection;
            XMMATRIX normaltransform = XMMatrixTranspose(XMMatrixInverse(nullptr, worldview));
            XMStoreFloat4x4(&m_vInstances[m_backBufferIndex][i][count].NormalTransform, XMMatrixTranspose(normaltransform));
            XMStoreFloat4x4(&m_vInstances[m_backBufferIndex][i][count].Transform, XMMatrixTranspose(transform));
            m_vInstances[m_backBufferIndex][i][count].MaterialIndex = obj.matind;
            count++;
         
        }
        
    }

    // upload de las constantes
    BYTE* data;

    // First, upload of pass constants.
    m_vConstantBuffer[m_backBufferIndex]->Map(0, nullptr, reinterpret_cast<void**>(&data)); // realizamos el mapeo
    //auto elementSizeConstants= CalcConstantBufferByteSize(sizeof(vConstants));
    
    memcpy(data, reinterpret_cast<const void*>(&passTransform), sizeof(vConstants));
    if (m_vConstantBuffer[m_backBufferIndex] != nullptr)
        m_vConstantBuffer[m_backBufferIndex]->Unmap(0, nullptr);

    // upload del buffer estructurado
    size_t elementSizeInstance = sizeof(vInstance);
    for(int i=0;i<m_objects.size();i++){
        size_t numberOfInstances = m_objects[i].size();
        if (numberOfInstances > 0) {
            m_vInstanceBuffer[m_backBufferIndex][i]->Map(0, nullptr, reinterpret_cast<void**>(&data)); // realizamos el mapeo
            //memcpy(&data[i * c_NumberOfInstancesPerObject * elementSizeInstance], reinterpret_cast<const void*>(m_vInstances[m_backBufferIndex][i].data()), numberOfInstances*elementSizeInstance); //Copia de la transformación
            memcpy(data, reinterpret_cast<const void*>(&m_vInstances[m_backBufferIndex][i][0]), numberOfInstances * elementSizeInstance); //Copia de la transformación
            if (m_vInstanceBuffer[m_backBufferIndex][i] != nullptr)
                m_vInstanceBuffer[m_backBufferIndex][i]->Unmap(0, nullptr);
        }
       
    }
    

    
    


    elapsedTime;
}

XMMATRIX Game::UpdateView() {

    XMVECTOR location = m_Position;
    XMVECTOR target = location + m_LookDirection;
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(location, target, up);
    XMStoreFloat4x4(&m_view, view);
    return view;

}

void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    Clear();

    // TODO: Add your rendering code here.
    // First, we re-connect root parameters for the currente m_backBufferIndex.
    CD3DX12_GPU_DESCRIPTOR_HANDLE cHandle(m_cDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    cHandle.Offset(2*m_backBufferIndex , m_cDescriptorSize);
    m_commandList->SetGraphicsRootDescriptorTable(0, // número de root parameter
            cHandle); //manejador a la posición del heap donde comenzaría el rango de descriptores
    cHandle.Offset(1, m_cDescriptorSize);
    m_commandList->SetGraphicsRootDescriptorTable(1, // número de root parameter
        cHandle);

    D3D12_VERTEX_BUFFER_VIEW vView[1] = { m_vBufferView };
    D3D12_INDEX_BUFFER_VIEW iView[1] = { m_iBufferView };
    m_commandList->IASetVertexBuffers(0, 1, vView);
    m_commandList->IASetIndexBuffer(iView);
    //--------------------------------------------------------------------------------------
    // Now Draw IndexedInstanced Data
    UINT indexStart = 0;
    UINT vertexStart = 0;

    UINT startInDescriptorHeap = static_cast<UINT>((1 + c_NumberOfObjects) * m_backBufferIndex);
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbHandle(m_cDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    cbHandle.Offset(startInDescriptorHeap, m_cDescriptorSize);
    m_commandList->SetGraphicsRootDescriptorTable(0, // para pass constant
        cbHandle);

    for (int ishape = 0; ishape < m_meshes.size();ishape++) {
        UINT numberOfIndex = static_cast<UINT>(m_meshes[ishape]->indices.size());
        UINT numberOfInstances = static_cast<UINT>(m_objects[ishape].size());
        UINT numberOfVertices = static_cast<UINT>(m_meshes[ishape]->vertices.size());
        if (m_objects[ishape].size() > 0) { // If there are instances


            CD3DX12_GPU_DESCRIPTOR_HANDLE saHandle(m_cDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
            saHandle.Offset(startInDescriptorHeap + 1 + ishape, m_cDescriptorSize);
            m_commandList->SetGraphicsRootDescriptorTable(1, // para instance constant
                saHandle);

            //TODO: reconexión de las texturas por objeto

            if (m_objects[ishape].size() > 0) {
                m_commandList->DrawIndexedInstanced(numberOfIndex, 
                    numberOfInstances, indexStart, vertexStart, 0);
            }

        }
        
        indexStart += numberOfIndex ;
        vertexStart += numberOfVertices;
    
    
    }

    // Send the command list off to the GPU for processing.
    DX::ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));
    // Now RenderUI
    RenderUI();
    //m_d3d11DeviceContext->Flush(); // comming back to d3d12 requires flush
    // Show the new frame.
     // Transition the render target to the state that allows it to be presented to the display.
    //D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    //m_commandList->ResourceBarrier(1, &barrier);
    
   
    Present();
}
// Helper method to prepare the command list for rendering and clear the back buffers.
void Game::Clear()
{
    // Reset command list and allocator.
    DX::ThrowIfFailed(m_commandAllocators[m_backBufferIndex]->Reset());
    DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_backBufferIndex].Get(), m_pso.Get())); // Nota: establecer el PSO.

    // Transition the render target into the correct state to allow for drawing into it.
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    // Clear the views.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_backBufferIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptor(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    m_commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    m_commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
    D3D12_RECT scissorRect = { 0, 0, m_outputWidth, m_outputHeight };
    m_commandList->RSSetViewports(1, &viewport);
    m_commandList->RSSetScissorRects(1, &scissorRect);

    /*  Establecemos en el pipeline la root signauture:
    La utilización de la root signature lleva tres acciones en la lista de comandos:
    a) Establecer la root signature
    b) Establecer un array de heaps de descriptores: en este caso el root parameter 0, es un descriptor table.
    Esto es un rango de descriptores, procedentes de un heap de descriptores.
    c) Establecer el punto de comienzo del rango de descriptores a usar (se referencia a partir del heap anterior)

    La lista de comandos que haga el Draw debe establecer la root signature
    */
    // Subtarea a: Establecer la root signature
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    // Subtarea b: b Establecer un array de descriptor heaps
    ID3D12DescriptorHeap* arrayHeaps[] = { m_cDescriptorHeap.Get(), m_sDescriptorHeap.Get()};
    m_commandList->SetDescriptorHeaps(_countof(arrayHeaps), arrayHeaps);

    // Subtarea c Establece el punto de comienzo del rango de descriptores.

   
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_cDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    srvHandle.Offset((1+static_cast<UINT>(c_NumberOfObjects))*c_swapBufferCount, m_cDescriptorSize);
    m_commandList->SetGraphicsRootDescriptorTable(2, // para SRV Textura
        srvHandle);

    CD3DX12_GPU_DESCRIPTOR_HANDLE sHandle(m_sDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootDescriptorTable(3, // número de root parameter
        sHandle); //manejador a la posición del heap donde comenzaría el rango de descriptores
    
     // Todo: Establecemos la vista para el buffer de indices

     // Es necesario pasar un array de buffer views
    D3D12_VERTEX_BUFFER_VIEW vView[1] = { m_vBufferView };
    D3D12_INDEX_BUFFER_VIEW iView[1] = { m_iBufferView };

    m_commandList->IASetVertexBuffers(0, 1, vView);

    
    m_commandList->IASetIndexBuffer(iView);


    /* Establecemos la topología: es obligatorio*/
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);




}

// Submits the command list to the GPU and presents the back buffer contents to the screen.
void Game::Present()
{
    // Transition the render target to the state that allows it to be presented to the display.
    //D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    //m_commandList->ResourceBarrier(1, &barrier);

    // Send the command list off to the GPU for processing.
    //DX::ThrowIfFailed(m_commandList->Close());
    //m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);

        MoveToNextFrame();
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended.
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed.
}

void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);
    m_outputRotation = rotation;

    CreateResources();

    // TODO: Game window is being resized.
}

void Game::ValidateDevice()
{
    // The D3D Device is no longer valid if the default adapter changed since the device
    // was created or if the device has been removed.

    DXGI_ADAPTER_DESC previousDesc;
    {
        ComPtr<IDXGIAdapter1> previousDefaultAdapter;
        DX::ThrowIfFailed(m_dxgiFactory->EnumAdapters1(0, previousDefaultAdapter.GetAddressOf()));

        DX::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));
    }

    DXGI_ADAPTER_DESC currentDesc;
    {
        ComPtr<IDXGIFactory4> currentFactory;
        DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(currentFactory.GetAddressOf())));

        ComPtr<IDXGIAdapter1> currentDefaultAdapter;
        DX::ThrowIfFailed(currentFactory->EnumAdapters1(0, currentDefaultAdapter.GetAddressOf()));

        DX::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));
    }

    // If the adapter LUIDs don't match, or if the device reports that it has been removed,
    // a new D3D device must be created.

    if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart
        || previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart
        || FAILED(m_d3dDevice->GetDeviceRemovedReason()))
    {
        // Create a new device and swap chain.
        OnDeviceLost();
    }
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
        }
    }
#endif

    DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));

    ComPtr<IDXGIAdapter1> adapter;
    GetAdapter(adapter.GetAddressOf());

    // Create the DX12 API device object.
    DX::ThrowIfFailed(D3D12CreateDevice(
        adapter.Get(),
        m_featureLevel,
        IID_PPV_ARGS(m_d3dDevice.ReleaseAndGetAddressOf())
        ));

#ifndef NDEBUG
    // Configure debug device (if active).
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(m_d3dDevice.As(&d3dInfoQueue)))
    {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
#endif

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    DX::ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf())));

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = c_swapBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

    m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        DX::ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocators[n].ReleaseAndGetAddressOf())));
    }

    // Create a command list for recording graphics commands.
    DX::ThrowIfFailed(m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
    
    //DX::ThrowIfFailed(m_commandList->Close());

    // Create a fence for tracking GPU execution progress.
    DX::ThrowIfFailed(m_d3dDevice->CreateFence(m_fenceValues[m_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    m_fenceValues[m_backBufferIndex]++;

    m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_fenceEvent.IsValid())
    {
        throw std::exception("CreateEvent");
    }

    // TODO: Initialize device dependent objects here (independent of window size).

    // D3D1211OND312 // From samples developed by Microsoft
    UINT dxgiFactoryFlags = 0;
    UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};

    // Create an 11 device wrapped around the 12 device and share
     // 12's command queue.
    
    DX::ThrowIfFailed(D3D11On12CreateDevice(
        m_d3dDevice.Get(),
        d3d11DeviceFlags,
        nullptr,
        0,
        reinterpret_cast<IUnknown**>(m_commandQueue.GetAddressOf()),
        1,
        0,
        &m_d3d11Device,
        &m_d3d11DeviceContext,
        nullptr
    ));

    // Query the 11On12 device from the 11 device.
    DX::ThrowIfFailed(m_d3d11Device.As(&m_d3d11On12Device));

    // Create D2D/DWrite components.
    {
        D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
        DX::ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &m_d2dFactory));
        ComPtr<IDXGIDevice> dxgiDevice;
        DX::ThrowIfFailed(m_d3d11On12Device.As(&dxgiDevice));
        DX::ThrowIfFailed(m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice));
        DX::ThrowIfFailed(m_d2dDevice->CreateDeviceContext(deviceOptions, &m_d2dDeviceContext));
        DX::ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_dWriteFactory));
    }

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Release resources that are tied to the swap chain and update fence values.
    unsigned int cWrappedBuffers=0;
    unsigned int cD2DRenderTargets=0;
    unsigned int cRenderTargets = 0;
   
       

    if(m_d3d11DeviceContext)
        m_d3d11DeviceContext->ClearState();

    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        //Microsoft::WRL::ComPtr<IDXGISurface> surface;
        //DX::ThrowIfFailed(m_wrappedBackBuffers[n].As(&surface));
        //m_wrappedBackBuffers[n].Get()->Release();
        cWrappedBuffers= m_wrappedBackBuffers[n].Reset();
        cRenderTargets = m_renderTargets[n].Reset();
       
        m_fenceValues[n] = m_fenceValues[m_backBufferIndex];
    }
    
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);

    
    if (m_swapChain)
    {
        
        HRESULT hr = m_swapChain->ResizeBuffers(c_swapBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = c_swapBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        // Create a swap chain for the window.
        ComPtr<IDXGISwapChain1> swapChain;
        DX::ThrowIfFailed(m_dxgiFactory->CreateSwapChainForCoreWindow(
            m_commandQueue.Get(),
            m_window,
            &swapChainDesc,
            nullptr,
            swapChain.GetAddressOf()
            ));

        DX::ThrowIfFailed(swapChain.As(&m_swapChain));
    }

    DX::ThrowIfFailed(m_swapChain->SetRotation(m_outputRotation));

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        DX::ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(m_renderTargets[n].GetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_renderTargets[n]->SetName(name);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(n), m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvDescriptor);

        // Create a wrapped 11On12 resource of this back buffer. Since we are 
            // rendering all D3D12 content first and then all D2D content, we specify 
            // the In resource state as RENDER_TARGET - because D3D12 will have last 
            // used it in this state - and the Out resource state as PRESENT. When 
            // ReleaseWrappedResources() is called on the 11On12 device, the resource 
            // will be transitioned to the PRESENT state.
        
            unsigned int count = 0;
            m_renderTargets[n].Get()->AddRef();
            count = m_renderTargets[n].Get()->Release();
            D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
            DX::ThrowIfFailed(m_d3d11On12Device->CreateWrappedResource(
                m_renderTargets[n].Get(),
                &d3d11Flags,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT,
                IID_PPV_ARGS(m_wrappedBackBuffers[n].GetAddressOf())
            ));
            m_renderTargets[n].Get()->AddRef();
            count = m_renderTargets[n].Get()->Release();

        
       

        // Create a render target for D2D to draw directly to this back buffer.
        
        float ldpi = winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView().LogicalDpi();
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
            ldpi,
            ldpi
        );
        

        
        Microsoft::WRL::ComPtr<IDXGISurface> surface;
        DX::ThrowIfFailed(m_wrappedBackBuffers[n].As(&surface));
        DX::ThrowIfFailed(m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
            surface.Get(),
            &bitmapProperties,
            &m_d2dRenderTargets[n]
        ));
        

    }

    // Reset the index to the current back buffer.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
    // on this surface.
    CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        depthBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This depth stencil view has only one texture.
        1  // Use a single mipmap level.
        );
    depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = depthBufferFormat;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &depthHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
        ));

    m_depthStencil->SetName(L"Depth stencil");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthBufferFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // TODO: Initialize windows-size dependent objects here.
}

void Game::WaitForGpu()
{
    // Schedule a Signal command in the GPU queue.
    DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_backBufferIndex]));

    // Wait until the Signal has been processed.
    DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_backBufferIndex], m_fenceEvent.Get()));
    WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_backBufferIndex]++;
}

void Game::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[m_backBufferIndex];
    DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the back buffer index.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_backBufferIndex])
    {
        DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_backBufferIndex], m_fenceEvent.Get()));
        WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_backBufferIndex] = currentFenceValue + 1;
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP. Otherwise throw an exception.
void Game::GetAdapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf()); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        DX::ThrowIfFailed(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), m_featureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

#if !defined(NDEBUG)
    if (!adapter)
    {
        if (FAILED(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
        {
            throw std::exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }
    }
#endif

    if (!adapter)
    {
        throw std::exception("No Direct3D 12 device found");
    }

    *ppAdapter = adapter.Detach();
}

void Game::OnDeviceLost()
{
    // TODO: Perform Direct3D resource cleanup.

    for (UINT n = 0; n < c_swapBufferCount; n++)
    {
        m_commandAllocators[n].Reset();
        m_renderTargets[n].Reset();
    }

    m_depthStencil.Reset();
    m_fence.Reset();
    m_commandList.Reset();
    m_swapChain.Reset();
    m_rtvDescriptorHeap.Reset();
    m_dsvDescriptorHeap.Reset();
    m_commandQueue.Reset();
    m_d3dDevice.Reset();
    m_dxgiFactory.Reset();

    CreateDevice();
    CreateResources();
}

void Game::CreateMainInputFlowResources(const std::vector<std::shared_ptr<Mesh>> &vMesh) {

    /*
    Objetivo 1.
    Comenzamos por preparar los buffers de vértices y de índices. Estos buffers se van a cargar
    con datos al principio, y luego no se van a actualizar por cada frame. Por eso es mejor
    crearlos en un heap de tipo DEFAULT al que accede con mayor eficiencia la GPU. Por contra
    la CPU no puede cargar datos en un DEFAULT. Entonces hay que crear un segundo buffer en un heap
    de tipo UPLOAD y transferir del UPLOAD al DEFAULT. Los buffer de vértices e índices no requieren
    un heap de descriptores, se conectan al pipeline directamente con una vista.

    */

    size_t numberOfMeshes = vMesh.size();

    /*Tarea 1: Creación de los buffers.*/
    D3D12_HEAP_PROPERTIES heapProperties;
    D3D12_RESOURCE_DESC resourceDescription;
    size_t vSize = 0; // Total size of resource for vertices
    size_t iSize = 0; // Total size of resource for indices
    for (int indMesh = 0; indMesh < numberOfMeshes; indMesh++) {
        //GameStatics::ShapeName shapeName = static_cast<GameStatics::ShapeName>(indMesh);
        std::shared_ptr<Mesh> mesh = vMesh[indMesh];
        vSize += mesh->GetVSize();
        iSize += mesh->GetISize();
    }

    // Creación de los resource buffers default y upload para los vértices
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(vSize);
    DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
         &heapProperties,
         D3D12_HEAP_FLAG_NONE,
         &resourceDescription,
         D3D12_RESOURCE_STATE_COMMON,
         nullptr,
         IID_PPV_ARGS(m_vBufferDefault.GetAddressOf())));

        // Ahora un buffer upload, de esta forma tenemos un puente upload para pasar a default.
     heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
     DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
           &heapProperties,
           D3D12_HEAP_FLAG_NONE,
           &resourceDescription,
           D3D12_RESOURCE_STATE_GENERIC_READ,
           nullptr,
           IID_PPV_ARGS(m_vBufferUpload.GetAddressOf())));

     // Creación de los buffers default y upload para los índices
     heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
     resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(iSize);
     DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
          &heapProperties,
          D3D12_HEAP_FLAG_NONE,
          &resourceDescription,
          D3D12_RESOURCE_STATE_COMMON,
          nullptr,
          IID_PPV_ARGS(m_iBufferDefault.GetAddressOf())));

     // Ahora un buffer upload, de esta forma tenemos un puente upload para pasar a default.

     heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
     resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(iSize);
     DX::ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
           &heapProperties,
           D3D12_HEAP_FLAG_NONE,
           &resourceDescription,
           D3D12_RESOURCE_STATE_GENERIC_READ,
           nullptr,
           IID_PPV_ARGS(m_iBufferUpload.GetAddressOf())));

    /*Tarea 2: Preparamos el origen de los datos de vértices e índices.*/
     
     D3D12_SUBRESOURCE_DATA origen_vertices_struct;
     origen_vertices_struct.pData = nullptr;
     origen_vertices_struct.RowPitch = 0;
     origen_vertices_struct.SlicePitch = 0;
     D3D12_SUBRESOURCE_DATA origen_indices_struct;
     origen_indices_struct.pData = nullptr;
     origen_indices_struct.RowPitch = 0;
     origen_indices_struct.SlicePitch = 0;

     std::vector<Vertex> vertexOrigin;
     std::vector<UINT> indexOrigin;
     // Concatenate vertex and index data
     for (int indMesh = 0; indMesh < numberOfMeshes; indMesh++) {

         std::shared_ptr<Mesh> mesh = vMesh[indMesh];
         vertexOrigin.insert(vertexOrigin.end(), mesh->vertices.begin(), mesh->vertices.end());
         indexOrigin.insert(indexOrigin.end(), mesh->indices.begin(), mesh->indices.end());
         
         origen_vertices_struct.RowPitch += mesh->GetVSize();
         origen_indices_struct.RowPitch += mesh->GetISize();
        
     }

     origen_vertices_struct.pData = vertexOrigin.data();
     origen_vertices_struct.SlicePitch = origen_vertices_struct.RowPitch;
     origen_indices_struct.pData = indexOrigin.data();
     origen_indices_struct.SlicePitch = origen_indices_struct.RowPitch;
        /*Tarea 3: Realizamos la transferencia desde el origen hasta el buffer DEFAULT pasando por el buffer UPLOAD*/
        /*Vértices*/
        // Cambio de estado en el recurso de destino
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_vBufferDefault.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            m_commandList->ResourceBarrier(1, &barrier);

    // Ordenamos la transferencia vertices
        UpdateSubresources<1>(m_commandList.Get(), m_vBufferDefault.Get(), m_vBufferUpload.Get(), 0,0, 1, &origen_vertices_struct);
    // Cambio de estado en el recurso de destino
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_vBufferDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
            m_commandList->ResourceBarrier(1, &barrier);

    /*Índices*/
    // Cambio de estado en el recurso de destino
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_iBufferDefault.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            m_commandList->ResourceBarrier(1, &barrier);

        // Ordenamos la transferencia vertices
        UpdateSubresources<1>(m_commandList.Get(), m_iBufferDefault.Get(), m_iBufferUpload.Get(), 0, 0, 1, &origen_indices_struct);
        
        // Cambio de estado en el recurso de destino
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_iBufferDefault.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
        m_commandList->ResourceBarrier(1, &barrier);

        // The array of D3D12_SUBRESOURCE_DATA structures can deleted
       
    
    /* Tarea 3: Establecemos una vista para vértices e índices*/
    // Establecemos la vista (descriptor) para el buffer de vértices
    /* Vértices*/

        D3D12_GPU_VIRTUAL_ADDRESS vBufferStart = m_vBufferDefault->GetGPUVirtualAddress();
        D3D12_GPU_VIRTUAL_ADDRESS iBufferStart = m_iBufferDefault->GetGPUVirtualAddress();
        
        m_vBufferView.BufferLocation = vBufferStart;
        m_vBufferView.StrideInBytes = sizeof(Vertex);
        m_vBufferView.SizeInBytes = 0;
        m_iBufferView.BufferLocation = iBufferStart;
        m_iBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_iBufferView.SizeInBytes = 0;

        for (int indMesh=0; indMesh < numberOfMeshes; indMesh++) {
            
           
            std::shared_ptr<Mesh> mesh = vMesh[indMesh];

            
            m_vBufferView.SizeInBytes += mesh->GetVSize();

            m_iBufferView.SizeInBytes += mesh->GetISize();

            
        }

        D3D12_VERTEX_BUFFER_VIEW aVBufferView[1] = { m_vBufferView };  // Es necesario pasar un array de buffer views
        D3D12_INDEX_BUFFER_VIEW aIBufferView[1] = { m_iBufferView };

        m_commandList->IASetVertexBuffers(0, 1, aVBufferView);
        m_commandList->IASetIndexBuffer(aIBufferView);

    
    /* Tarea 4: Cargamos las texturas de la malla*/
        // Load textures in RT0, RT1, ....
        size_t numTextures = GameStatics::TexFileNames.size();
        m_textureDefault.resize(numTextures);
        m_textureUpload.resize(numTextures);
        for (auto it = GameStatics::TexFileNames.begin(); it != GameStatics::TexFileNames.end(); it++) {
            auto texName = it->first;
            auto fileName = it->second;
            unsigned int texIdx = static_cast<unsigned int>(texName);
            DX::ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_d3dDevice.Get(), m_commandList.Get(),
                fileName.c_str(),
                m_textureDefault[texIdx],
                m_textureUpload[texIdx]));
        }
        
    /*------------------------- Fin Objetivo 1  ----------------------------------------------------------------------------*/

    // Creación de recursos para las constantes

    // Constantes comunes
    // Pass constants: RC0, RC1, RC2
    unsigned int elementSizeConstants = CalcConstantBufferByteSize(sizeof(vConstants));

    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(elementSizeConstants);
    for (int i = 0; i < c_swapBufferCount; i++) {
        m_d3dDevice->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescription,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_vConstantBuffer[i].GetAddressOf())
        );
    }

    // Constantes por objeto
    // Resources for RS00, RS01, ..., RS10,RS11, ..., RS20, RS21,...
    // RSij is buffer resource for frame resource i, and object j. 
    // Each RSij is for object instances Oj0, Oj1, ... Ojn.
    // Number of instances
    size_t numberOfInstances = 0;
    numberOfInstances = c_NumberOfInstancesPerObject;
    for (int i = 0; i < c_swapBufferCount; i++) {
        m_vInstanceBuffer[i].clear();
        m_vInstanceBuffer[i].resize(c_NumberOfObjects);

        for (int j = 0; j < c_NumberOfObjects; j++) {
            unsigned int instanceBufferSize = CalcConstantBufferByteSize(
                static_cast<unsigned int>(sizeof(vInstance) * numberOfInstances));
            //unsigned int instanceBufferSize = (sizeof(vInstance) * numberOfInstances);
            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);

            m_d3dDevice->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDescription,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(m_vInstanceBuffer[i][j].GetAddressOf())
            );

        }

    }
   


    /* Tarea 2: crear un heap de descriptores CBV_SRV_UAV*/

    D3D12_DESCRIPTOR_HEAP_DESC cHeapDescriptor;
    cHeapDescriptor.NumDescriptors = static_cast<UINT>((1+c_NumberOfObjects)*c_swapBufferCount+numTextures); // (CBV(Pass) + (number of Objects)* SRV(Instance)) por swap buffer + num of textures
    cHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cHeapDescriptor.NodeMask = 0;

    m_d3dDevice->CreateDescriptorHeap(&cHeapDescriptor, IID_PPV_ARGS(&m_cDescriptorHeap));

    /* Tarea 3: Crear un heap de descriptores para samplers*/
    D3D12_DESCRIPTOR_HEAP_DESC descHeapSampler = {};
    descHeapSampler.NumDescriptors = 1;
    descHeapSampler.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    descHeapSampler.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    DX::ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&descHeapSampler, IID_PPV_ARGS(&m_sDescriptorHeap)));


   // Create views for CBV_SRV_UAV resources in the CBV_SRV_UAV Heap.

    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(
        m_cDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
    );

    m_cDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    
    for (int i = 0; i < c_swapBufferCount; i++) {
        // View for buffer of common constants for all objects: DCiRCi
        D3D12_GPU_VIRTUAL_ADDRESS cAddress = m_vConstantBuffer[i]->GetGPUVirtualAddress();
        D3D12_CONSTANT_BUFFER_VIEW_DESC cDescriptor;
        cDescriptor.BufferLocation = cAddress;
        cDescriptor.SizeInBytes = elementSizeConstants;
        m_d3dDevice->CreateConstantBufferView(&cDescriptor, hDescriptor);
        hDescriptor.Offset(1, m_cDescriptorSize);
        
        
        // Views for buffer of constants per object: DSiRSi0, DSiRSi1, ... (one view por object)
        
        for (int j = 0; j < m_vInstanceBuffer[i].size(); j++) {
            cAddress = m_vInstanceBuffer[i][j]->GetGPUVirtualAddress();
            D3D12_SHADER_RESOURCE_VIEW_DESC sBDesc = {};
            sBDesc = {};
            sBDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            sBDesc.Format = m_vInstanceBuffer[i][j].Get()->GetDesc().Format;
            sBDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            sBDesc.Buffer.FirstElement = 0;
            sBDesc.Buffer.NumElements = static_cast<UINT>(c_NumberOfInstancesPerObject);
            sBDesc.Buffer.StructureByteStride = sizeof(vInstance);
            sBDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

            m_d3dDevice->CreateShaderResourceView(m_vInstanceBuffer[i][j].Get(), &sBDesc, hDescriptor);
            hDescriptor.Offset(1, m_cDescriptorSize);
        }
           
    }

        /* Tarea 5 Creamos descriptores SRV para las texturas*/
    
        // Finally we create view for textures in the same CBV_SRV_UAV Descriptor Heap
     for (int i = 0; i < numTextures; i++) {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = m_textureDefault[i]->GetDesc().Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = -1;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            m_d3dDevice->CreateShaderResourceView(m_textureDefault[i].Get(), &srvDesc, hDescriptor);
            hDescriptor.Offset(1, m_cDescriptorSize);
      }

    /* Tarea 6 Creamoes un descriptor para el sampler*/
    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    m_d3dDevice->CreateSampler(&samplerDesc, m_sDescriptorHeap->GetCPUDescriptorHandleForHeapStart());






    /*
    Objetivo 3: Creamos una root signature ya que usaremos registros de los shaders conectados a recursos y
    la configuramos para ser usada por el pipeline
    */
    // Establecemos la root signature

/* Tarea 1: Crear un array de root parameters*/

    CD3DX12_ROOT_PARAMETER rootParameters[4]; //Array de root parameters // CBT
    // Creamos un rango de tablas de descriptores
    CD3DX12_DESCRIPTOR_RANGE descRange[4]; // CBT
    descRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); //1 CB to slot 0
    descRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1); //1 SRV for SB to Slot 0, space 1 f
    descRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // 1 SRV to Slot 0, space 0 for texture
    descRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // 1 Sampler to Slot 0


    rootParameters[0].InitAsDescriptorTable(1, // Número de rangos 
        &descRange[0],D3D12_SHADER_VISIBILITY_ALL); 
    rootParameters[1].InitAsDescriptorTable(1, // Número de rangos 
        &descRange[1], D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[2].InitAsDescriptorTable(1, // Número de rangos 
        &descRange[2], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, // Número de rangos 
        &descRange[3], D3D12_SHADER_VISIBILITY_PIXEL);

/* Tarea 2: Creamos una descripción de la root signature y la serializamos */
    // Descripción de la root signature //CBT
    CD3DX12_ROOT_SIGNATURE_DESC rsDescription(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    //Debemos serializar la descripción root signature
    Microsoft::WRL::ComPtr<ID3DBlob> serializado = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> error = nullptr;
    D3D12SerializeRootSignature(&rsDescription, D3D_ROOT_SIGNATURE_VERSION_1, serializado.GetAddressOf(), error.GetAddressOf());

    /* Tarea 3: Creamos la root signature*/
        //Con la descripción serializada creamos el componente ID3DRootSignature
    DX::ThrowIfFailed(m_d3dDevice->CreateRootSignature(
        0,
        serializado->GetBufferPointer(),
        serializado->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));





    // Cerramos la lista de comandos, lanzamos la ejeución de los mismos

    m_commandList->Close();


    m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));


    DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), 1));

    // Wait until the Signal has been processed.
    DX::ThrowIfFailed(m_fence->SetEventOnCompletion(1, m_fenceEvent.Get()));
    WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);


    // Create D2D/DWrite objects for rendering text.
    {
        DX::ThrowIfFailed(m_d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_textBrush));
        DX::ThrowIfFailed(m_dWriteFactory->CreateTextFormat(
            L"Verdana",
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            50,
            L"en-us",
            &m_textFormat
        ));
        DX::ThrowIfFailed(m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
        DX::ThrowIfFailed(m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
    }

}

void Game::LoadPrecompiledShaders() {

    DX::ThrowIfFailed(
        D3DReadFileToBlob(L"vertex.cso", m_vsByteCode.GetAddressOf()));

    DX::ThrowIfFailed(
        D3DReadFileToBlob(L"pixel.cso", m_psByteCode.GetAddressOf()));


    m_vs = { reinterpret_cast<char*>(m_vsByteCode->GetBufferPointer()),
            m_vsByteCode->GetBufferSize() };

    m_ps = { reinterpret_cast<char*>(m_psByteCode->GetBufferPointer()),
            m_psByteCode->GetBufferSize() };

}

void Game::PSO()
{

    // Input Layout
    // 1. HLSL Semantics
    // 2. Semantic Index.
    // 3. DXGI Format.
    // 4. Input Slot: IA Stage has 15 slots.
    // 5. Offset in bytes from the begining of the vertex information.
    // 6. Input class for this slot. There are two possibilites: each data unit represents a vertex of the same instance
    // or each data unit represents  data for an instance.
    // 7. Instance data step rate. Indicate how many instances have to be drawn befor advancing to the next data
    // This parameter should be zero if we are in D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
    // Input data per instance is a way to have instance particular parameters when drawing multiple instances
    //

    m_inputLayout = {

        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
    {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,28,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
    {"UV",0,DXGI_FORMAT_R32G32_FLOAT,0,40,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"MATINDEX",0,DXGI_FORMAT_R32_UINT,0,48,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };



    // Rasterizer state
    CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;

    ZeroMemory(&m_psoDescriptor, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));


    m_psoDescriptor.InputLayout = { m_inputLayout.data(), (unsigned int)m_inputLayout.size() };
    m_psoDescriptor.pRootSignature = m_rootSignature.Get();
    m_psoDescriptor.VS = { m_vs.pShaderBytecode,m_vs.BytecodeLength };
    m_psoDescriptor.PS = { m_ps.pShaderBytecode,m_ps.BytecodeLength };
    m_psoDescriptor.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    //m_psoDescriptor.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    //m_psoDescriptor.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    m_psoDescriptor.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    m_psoDescriptor.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    m_psoDescriptor.SampleMask = UINT_MAX;
    m_psoDescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    m_psoDescriptor.NumRenderTargets = 1;
    m_psoDescriptor.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
    m_psoDescriptor.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    m_psoDescriptor.SampleDesc.Count = 1;
    m_psoDescriptor.SampleDesc.Quality = 0;

    m_d3dDevice->CreateGraphicsPipelineState(&m_psoDescriptor, IID_PPV_ARGS(&m_pso));



}

void Game::RenderUI()
{
    D2D1_SIZE_F rtSize = m_d2dRenderTargets[m_backBufferIndex]->GetSize();
    D2D1_RECT_F textRect = D2D1::RectF(0, 0, rtSize.width, rtSize.height);
    //D2D1_RECT_F textRect = D2D1::RectF(0, 0, 1200, 900);
    static const WCHAR text[] = L"11On12";

    // Acquire our wrapped render target resource for the current back buffer.
    m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_backBufferIndex].GetAddressOf(), 1);

    // Render text directly to the back buffer.
    m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[m_backBufferIndex].Get());
    m_d2dDeviceContext->BeginDraw();
    m_d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
    m_d2dDeviceContext->DrawText(
        text,
        _countof(text) - 1,
        m_textFormat.Get(),
        &textRect,
        m_textBrush.Get()
    );
    DX::ThrowIfFailed(m_d2dDeviceContext->EndDraw());

    // Release our wrapped render target resource. Releasing 
    // transitions the back buffer resource to the state specified
    // as the OutState when the wrapped resource was created.
    //m_d2dDeviceContext->SetTarget(nullptr); //
    
    m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_backBufferIndex].GetAddressOf(), 1);
    
    //m_d2dDeviceContext->SetTarget(nullptr); //
    // Flush to submit the 11 command list to the shared command queue.
    m_d3d11DeviceContext->Flush();
}

void Game::SetDPI(float xdpi, float ydpi) {
    m_dpi.xdpi = xdpi;
    m_dpi.ydpi = ydpi;
}