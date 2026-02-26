#include "Renderer2D.h"
#include "Mesh.h"
#include "Shader.h"
#include <vector>
#include <array>

namespace ge {
namespace renderer {

    struct QuadVertex
    {
        Math::Vec3f Position;
        Math::Vec4f Color;
        Math::Vec2f TexCoord;
        float TexIndex;
        float TilingFactor;
    };

    struct Renderer2DData
    {
        static const uint32_t MaxQuads = 10000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32; // API dependent, but 32 is common

        std::shared_ptr<Shader> TextureShader;
        std::shared_ptr<Texture> WhiteTexture;

        uint32_t QuadIndexCount = 0;
        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr = nullptr;

        std::array<std::shared_ptr<Texture>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1; // 0 = white texture

        Math::Vec4f QuadVertexPositions[4];

        Renderer2DStatistics Stats;
    };

    static Renderer2DData s_Data;

    void Renderer2D::Init()
    {
        // 1. Create Shader
        s_Data.TextureShader = Shader::Create("../src/shaders/sprite_batch.vert", "../src/shaders/sprite_batch.frag");

        // 2. Create White Texture (1x1)
        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture = Texture::Create(1, 1, &whiteTextureData, sizeof(uint32_t));

        // 3. Setup Buffers
        s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

        // 4. Set Texture Slot 0 to White
        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

        // 5. Setup Quad local positions
        s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
        s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
        s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
        s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
    }

    void Renderer2D::Shutdown()
    {
        delete[] s_Data.QuadVertexBufferBase;
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        s_Data.TextureShader->Bind();
        s_Data.TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

        StartBatch();
    }

    void Renderer2D::EndScene()
    {
        Flush();
    }

    void Renderer2D::StartBatch()
    {
        s_Data.QuadIndexCount = 0;
        s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;
        s_Data.TextureSlotIndex = 1;
    }

    void Renderer2D::Flush()
    {
        if (s_Data.QuadIndexCount == 0)
            return;

        // Bind Textures
        for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
            s_Data.TextureSlots[i]->Bind(i);

        // TODO: Map Buffer / Update Buffer data
        // For now, this is a placeholder for the actual GPU draw call
        // We'll need a dynamic OpenGLMesh or similar.

        s_Data.Stats.DrawCalls++;
    }

    void Renderer2D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    void Renderer2D::DrawQuad(const Math::Vec2f& position, const Math::Vec2f& size, const Math::Vec4f& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const Math::Vec3f& position, const Math::Vec2f& size, const Math::Vec4f& color)
    {
        if (s_Data.QuadIndexCount >= s_Data.MaxIndices)
            NextBatch();

        const float textureIndex = 0.0f; // White Texture

        // We'll calculate the model matrix manually here for speed instead of using Mat4f
        // Or we use a simple loop.
        
        for (int i = 0; i < 4; i++)
        {
            s_Data.QuadVertexBufferPtr->Position = { 
                position.x + (s_Data.QuadVertexPositions[i].x * size.x),
                position.y + (s_Data.QuadVertexPositions[i].y * size.y),
                position.z
            };
            s_Data.QuadVertexBufferPtr->Color = color;
            s_Data.QuadVertexBufferPtr->TexCoord = { (i == 1 || i == 2) ? 1.0f : 0.0f, (i == 2 || i == 3) ? 1.0f : 0.0f };
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = 1.0f;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;
        s_Data.Stats.QuadCount++;
    }

    // (Omitted other overloads for brevity in this step)

    void Renderer2D::ResetStats()
    {
        memset(&s_Data.Stats, 0, sizeof(Renderer2DStatistics));
    }

    Renderer2DStatistics Renderer2D::GetStats()
    {
        return s_Data.Stats;
    }

} // namespace renderer
} // namespace ge
