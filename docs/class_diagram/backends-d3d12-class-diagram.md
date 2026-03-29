# Xelqoria.Backends.D3D12 Class Diagram

```mermaid
classDiagram
    direction TB

    namespace RHI {
        class IGraphicsContext {
            <<interface>>
        }

        class ITexture {
            <<interface>>
        }

        class IVertexBuffer {
            <<interface>>
        }

        class QuadTransform2D
    }

    namespace Backends_D3D12 {
        class D3D12TextureCreateDesc {
            +width uint32_t
            +height uint32_t
            +format DXGI_FORMAT
        }

        class D3D12Texture {
            -m_width uint32_t
            -m_height uint32_t
            +Initialize(device, desc)
            +UploadInitialData(...)
            +GetWidth() uint32_t
            +GetHeight() uint32_t
        }

        class D3D12VertexBuffer {
            +GetBufferSize() uint32_t
            +GetStrideSize() uint32_t
        }

        class D3D12GraphicsContext {
            -m_width uint32_t
            -m_height uint32_t
            -m_spriteVertexBuffer shared_ptr~IVertexBuffer~
            -m_quadTransform QuadTransform2D
            +Initialize(...)
            +CreateTextureFromFile(...)
            +BindTexture(...)
            +SetQuadTransform(...)
            +Draw(...)
            +DrawIndexed(...)
            +Resize(...)
        }
    }

    D3D12GraphicsContext ..|> IGraphicsContext
    D3D12Texture ..|> ITexture
    D3D12VertexBuffer ..|> IVertexBuffer

    D3D12GraphicsContext --> ITexture : creates or binds
    D3D12GraphicsContext --> IVertexBuffer : holds
    D3D12GraphicsContext --> QuadTransform2D : uses
    D3D12GraphicsContext --> D3D12Texture : creates
    D3D12GraphicsContext --> D3D12VertexBuffer : creates
    D3D12Texture --> D3D12TextureCreateDesc : initialized by
```
