#include "SceneViewRenderer.h"
#include "EditorCamera.h"
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>
#include <vector>


#pragma comment(lib, "d3dcompiler.lib")

namespace Forge {

SceneViewRenderer::SceneViewRenderer() = default;
SceneViewRenderer::~SceneViewRenderer() { Shutdown(); }

void SceneViewRenderer::Initialize(ID3D12Device *device,
                                   ID3D12DescriptorHeap *srvHeap) {
  m_Device = device;
  m_SrvHeap = srvHeap;

  // RTV Heap 생성 (1개)
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
  rtvHeapDesc.NumDescriptors = 1;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap));
  m_RtvHandle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();

  // DSV Heap 생성 (1개)
  D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
  dsvHeapDesc.NumDescriptors = 1;
  dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  m_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap));
  m_DsvHandle = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();

  std::cout << "[SceneViewRenderer] Initialized." << std::endl;

  // Create Grid resources
  CreateGridPSO();
  CreateGridGeometry();
}

void SceneViewRenderer::Shutdown() {
  ReleaseResources();
  m_RtvHeap.Reset();
  m_DsvHeap.Reset();
}

void SceneViewRenderer::Resize(int width, int height) {
  if (width <= 0 || height <= 0)
    return;

  // Only create once - resize disabled for stability
  // (GPU sync required for safe resize, too complex for PHASE 1)
  if (m_ColorRT != nullptr)
    return;

  m_Width = width;
  m_Height = height;

  CreateResources();
}

void SceneViewRenderer::CreateResources() {
  if (!m_Device || m_Width <= 0 || m_Height <= 0)
    return;

  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

  // 1. Color Render Target
  D3D12_RESOURCE_DESC rtDesc = {};
  rtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  rtDesc.Width = m_Width;
  rtDesc.Height = m_Height;
  rtDesc.DepthOrArraySize = 1;
  rtDesc.MipLevels = 1;
  rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  rtDesc.SampleDesc.Count = 1;
  rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

  D3D12_CLEAR_VALUE clearValue = {};
  clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  clearValue.Color[0] = 0.1f;
  clearValue.Color[1] = 0.1f;
  clearValue.Color[2] = 0.1f;
  clearValue.Color[3] = 1.0f;

  HRESULT hr = m_Device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &rtDesc,
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue,
      IID_PPV_ARGS(&m_ColorRT));
  if (FAILED(hr)) {
    std::cerr << "[SceneViewRenderer] Failed to create ColorRT" << std::endl;
    return;
  }

  // 2. Depth Render Target
  rtDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  clearValue.DepthStencil.Depth = 1.0f;
  clearValue.DepthStencil.Stencil = 0;

  hr = m_Device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &rtDesc,
      D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&m_DepthRT));
  if (FAILED(hr)) {
    std::cerr << "[SceneViewRenderer] Failed to create DepthRT" << std::endl;
    return;
  }

  // 3. Create RTV
  m_Device->CreateRenderTargetView(m_ColorRT.Get(), nullptr, m_RtvHandle);

  // 4. Create DSV
  m_Device->CreateDepthStencilView(m_DepthRT.Get(), nullptr, m_DsvHandle);

  // 5. Create SRV for ImGui (고정 슬롯 SRV_SLOT)
  UINT handleSize = m_Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle =
      m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
  cpuHandle.ptr += handleSize * SRV_SLOT;

  m_Device->CreateShaderResourceView(m_ColorRT.Get(), nullptr, cpuHandle);

  m_SrvHandle = m_SrvHeap->GetGPUDescriptorHandleForHeapStart();
  m_SrvHandle.ptr += handleSize * SRV_SLOT;

  std::cout << "[SceneViewRenderer] Resources created: " << m_Width << "x"
            << m_Height << std::endl;
}

void SceneViewRenderer::ReleaseResources() {
  m_ColorRT.Reset();
  m_DepthRT.Reset();
}

void SceneViewRenderer::Render(ID3D12GraphicsCommandList *commandList,
                               const EditorCamera *camera) {
  if (!m_ColorRT || !m_DepthRT)
    return;

  // Transition to Render Target
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Transition.pResource = m_ColorRT.Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  commandList->ResourceBarrier(1, &barrier);

  // Set Render Target
  commandList->OMSetRenderTargets(1, &m_RtvHandle, FALSE, &m_DsvHandle);

  // Clear
  float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
  commandList->ClearRenderTargetView(m_RtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(m_DsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f,
                                     0, 0, nullptr);

  // [PHASE 4] Render Grid
  if (camera && m_Width > 0 && m_Height > 0) {
    D3D12_VIEWPORT viewport = {0.0f, 0.0f, (float)m_Width, (float)m_Height,
                               0.0f, 1.0f};
    D3D12_RECT scissor = {0, 0, m_Width, m_Height};
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    if (m_GridPSO && m_GridRootSig && m_GridVB && m_GridVertexCount > 0) {
      commandList->SetPipelineState(m_GridPSO.Get());
      commandList->SetGraphicsRootSignature(m_GridRootSig.Get());

      float aspect = (float)m_Width / (float)m_Height;
      DirectX::XMMATRIX viewProj =
          camera->GetViewMatrix() * camera->GetProjectionMatrix(aspect);
      commandList->SetGraphicsRoot32BitConstants(0, 16, &viewProj, 0);

      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
      commandList->IASetVertexBuffers(0, 1, &m_GridVBV);
      commandList->DrawInstanced(m_GridVertexCount, 1, 0, 0);
    }
  }

  // Transition back to Shader Resource
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  commandList->ResourceBarrier(1, &barrier);
}

void SceneViewRenderer::CreateGridPSO() {
  if (!m_Device)
    return;

  // 1. Root Signature
  D3D12_ROOT_PARAMETER rootParameters[1] = {};
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
  rootParameters[0].Constants.ShaderRegister = 0;
  rootParameters[0].Constants.Num32BitValues = 16;
  rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

  D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {
      1, rootParameters, 0, nullptr,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};

  Microsoft::WRL::ComPtr<ID3DBlob> signature;
  Microsoft::WRL::ComPtr<ID3DBlob> error;
  HRESULT hr = D3D12SerializeRootSignature(
      &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  if (FAILED(hr)) {
    std::cerr << "[SceneViewRenderer] Failed to serialize root signature"
              << std::endl;
    return;
  }

  hr = m_Device->CreateRootSignature(0, signature->GetBufferPointer(),
                                     signature->GetBufferSize(),
                                     IID_PPV_ARGS(&m_GridRootSig));
  if (FAILED(hr)) {
    std::cerr << "[SceneViewRenderer] Failed to create root signature"
              << std::endl;
    return;
  }

  // 2. Compile Shaders
  Microsoft::WRL::ComPtr<ID3DBlob> vs;
  Microsoft::WRL::ComPtr<ID3DBlob> ps;

  hr = D3DCompileFromFile(L"Source/Editor/GridShader.hlsl", nullptr, nullptr,
                          "VSMain", "vs_5_0", 0, 0, &vs, &error);
  if (FAILED(hr)) {
    std::cerr << "[SceneViewRenderer] Failed to compile VS" << std::endl;
    return;
  }

  hr = D3DCompileFromFile(L"Source/Editor/GridShader.hlsl", nullptr, nullptr,
                          "PSMain", "ps_5_0", 0, 0, &ps, &error);
  if (FAILED(hr)) {
    std::cerr << "[SceneViewRenderer] Failed to compile PS" << std::endl;
    return;
  }

  // 3. Input Layout
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  // 4. PSO
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
  psoDesc.pRootSignature = m_GridRootSig.Get();
  psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
  psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};

  psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
  psoDesc.RasterizerState.DepthClipEnable = TRUE;

  psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
  psoDesc.BlendState.IndependentBlendEnable = FALSE;
  const D3D12_RENDER_TARGET_BLEND_DESC defaultRtbDesc = {
      FALSE,
      FALSE,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_LOGIC_OP_NOOP,
      D3D12_COLOR_WRITE_ENABLE_ALL,
  };
  for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    psoDesc.BlendState.RenderTarget[i] = defaultRtbDesc;

  psoDesc.DepthStencilState.DepthEnable = TRUE;
  psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  psoDesc.DepthStencilState.StencilEnable = FALSE;

  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  psoDesc.SampleDesc.Count = 1;

  hr =
      m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_GridPSO));
  if (FAILED(hr)) {
    std::cerr << "[SceneViewRenderer] Failed to create Grid PSO" << std::endl;
    return;
  }

  std::cout << "[SceneViewRenderer] Grid PSO Created." << std::endl;
}

void SceneViewRenderer::CreateGridGeometry() {
  if (!m_Device)
    return;

  struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
  };
  std::vector<Vertex> vertices;
  int size = 10;
  DirectX::XMFLOAT4 color = {0.3f, 0.3f, 0.3f, 1.0f};

  for (int i = -size; i <= size; i++) {
    vertices.push_back({{(float)i, 0, (float)-size}, color});
    vertices.push_back({{(float)i, 0, (float)size}, color});
    vertices.push_back({{(float)-size, 0, (float)i}, color});
    vertices.push_back({{(float)size, 0, (float)i}, color});
  }

  m_GridVertexCount = (UINT)vertices.size();
  size_t bufferSize = vertices.size() * sizeof(Vertex);

  D3D12_HEAP_PROPERTIES heapProps = {D3D12_HEAP_TYPE_UPLOAD};
  D3D12_RESOURCE_DESC bufferDesc = {};
  bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufferDesc.Width = bufferSize;
  bufferDesc.Height = 1;
  bufferDesc.DepthOrArraySize = 1;
  bufferDesc.MipLevels = 1;
  bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
  bufferDesc.SampleDesc.Count = 1;
  bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  m_Device->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_GridVB));

  void *pData;
  m_GridVB->Map(0, nullptr, &pData);
  memcpy(pData, vertices.data(), bufferSize);
  m_GridVB->Unmap(0, nullptr);

  m_GridVBV.BufferLocation = m_GridVB->GetGPUVirtualAddress();
  m_GridVBV.StrideInBytes = sizeof(Vertex);
  m_GridVBV.SizeInBytes = (UINT)bufferSize;

  std::cout << "[SceneViewRenderer] Grid Geometry Created." << std::endl;
}

} // namespace Forge
