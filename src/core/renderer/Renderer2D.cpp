#include "Renderer2D.h"
#include "Mesh.h"
#include "Shader.h"
#include <vector>
#include <array>

namespace ge {
namespace renderer {

    struct Renderer2DData
    {
        static const uint32_t MaxQuads = 10000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32;

        std::shared_ptr<Shader> TextureShader;
        std::shared_ptr<Texture> WhiteTexture;
        std::shared_ptr<Mesh> QuadMesh;

        uint32_t QuadIndexCount = 0;
        Vertex* QuadVertexBufferBase = nullptr;
        Vertex* QuadVertexBufferPtr = nullptr;

        std::array<std::shared_ptr<Texture>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1;

        Math::Vec4f QuadVertexPositions[4];

        Renderer2DStatistics Stats;
    };

    static Renderer2DData s_Data;

    void Renderer2D::Init()
    {
        s_Data.TextureShader = Shader::Create("../src/shaders/sprite_batch.vert", "../src/shaders/sprite_batch.frag");

        uint32_t whiteTextureData = 0xffffffff;
        s_Data.WhiteTexture = Texture::Create(1, 1, &whiteTextureData, sizeof(uint32_t));

        s_Data.QuadVertexBufferBase = new Vertex[s_Data.MaxVertices];

        // Setup common index buffer
        uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];
        uint32_t offset = 0;
        for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
        {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;
            quadIndices[i + 3] = offset + 2;
            quadIndices[i + 4] = offset + 3;
            quadIndices[i + 5] = offset + 0;
            offset += 4;
        }
        
        // We need a way to pass indices to CreateDynamic or SetIndices
        // For now, let's assume CreateDynamic can take initial indices or we add SetIndices
        // Actually, CreateDynamic just allocates. We'll use a specialized constructor or just SetData for indices too if needed.
        // But indices are static, so we can just create a Mesh with them.
        s_Data.QuadMesh = Mesh::CreateDynamic(s_Data.MaxVertices, s_Data.MaxIndices);
        s_Data.QuadMesh->SetIndices(quadIndices, s_Data.MaxIndices);
        
        delete[] quadIndices;

        s_Data.TextureSlots[0] = s_Data.WhiteTexture;

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

        uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
        s_Data.QuadMesh->SetData(s_Data.QuadVertexBufferBase, dataSize);

        for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
            s_Data.TextureSlots[i]->Bind(i);

        s_Data.QuadMesh->SetIndexCount(s_Data.QuadIndexCount);
        s_Data.QuadMesh->Draw();

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

        for (int i = 0; i < 4; i++)
        {
            s_Data.QuadVertexBufferPtr->Position[0] = position.x + (s_Data.QuadVertexPositions[i].x * size.x);
            s_Data.QuadVertexBufferPtr->Position[1] = position.y + (s_Data.QuadVertexPositions[i].y * size.y);
            s_Data.QuadVertexBufferPtr->Position[2] = position.z;
            
            s_Data.QuadVertexBufferPtr->Color[0] = color.x;
            s_Data.QuadVertexBufferPtr->Color[1] = color.y;
            s_Data.QuadVertexBufferPtr->Color[2] = color.z;
            s_Data.QuadVertexBufferPtr->Color[3] = color.w;

            s_Data.QuadVertexBufferPtr->TexCoord[0] = (i == 1 || i == 2) ? 1.0f : 0.0f;
            s_Data.QuadVertexBufferPtr->TexCoord[1] = (i == 2 || i == 3) ? 1.0f : 0.0f;
            
            s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
            s_Data.QuadVertexBufferPtr->TilingFactor = 1.0f;
            s_Data.QuadVertexBufferPtr++;
        }

        s_Data.QuadIndexCount += 6;
        s_Data.Stats.QuadCount++;
    }

    void Renderer2D::DrawQuad(const Math::Vec2f& position, const Math::Vec2f& size, const std::shared_ptr<Texture>& texture, const Math::Vec4f& tint)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, texture, tint);
    }

    void Renderer2D::DrawQuad(const Math::Vec3f& position, const Math::Vec2f& size, const std::shared_ptr<Texture>& texture, const Math::Vec4f& tint)
    {
        if (s_Data.QuadIndexCount >= s_Data.MaxIndices)
            NextBatch();

        float textureIndex = 0.0f;
        for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
        {
            if (*s_Data.TextureSlots[i] == *texture)
            {
                textureIndex = (float)i;
                break;
            }
        }

        if (textureIndex == 0.0f)
        {
            if (s_Data.TextureSlotIndex >= s_Data.MaxTextureSlots)
                NextBatch();

            textureIndex = (float)s_Data.TextureSlotIndex;
            s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
            s_Data.TextureSlotIndex++;
        }

        for (int i = 0; i < 4; i++)
        {
            s_Data.QuadVertexBufferPtr->Position[0] = position.x + (s_Data.QuadVertexPositions[i].x * size.x);
            s_Data.QuadVertexBufferPtr->Position[1] = position.y + (s_Data.QuadVertexPositions[i].y * size.y);
            s_Data.QuadVertexBufferPtr->Position[2] = position.z;
            
            s_Data.QuadVertexBufferPtr->Color[0] = tint.x;
            s_Data.QuadVertexBufferPtr->Color[1] = tint.y;
            s_Data.QuadVertexBufferPtr->Color[2] = tint.z;
            s_Data.QuadVertexBufferPtr->Color[3] = tint.w;

            s_Data.QuadVertexBufferPtr->TexCoord[0] = (i == 1 || i == 2) ? 1.0f : 0.0f;
            s_Data.QuadVertexBufferPtr->TexCoord[1] = (i == 2 || i == 3) ? 1.0f : 0.0f;
            
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
