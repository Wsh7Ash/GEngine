#include "Model.h"
#include "../debug/log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace ge {
namespace renderer {

    // Conversion helper
    static Math::Mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
    {
        Math::Mat4 to;
        to.cols[0][0] = from.a1; to.cols[0][1] = from.b1; to.cols[0][2] = from.c1; to.cols[0][3] = from.d1;
        to.cols[1][0] = from.a2; to.cols[1][1] = from.b2; to.cols[1][2] = from.c2; to.cols[1][3] = from.d2;
        to.cols[2][0] = from.a3; to.cols[2][1] = from.b3; to.cols[2][2] = from.c3; to.cols[2][3] = from.d3;
        to.cols[3][0] = from.a4; to.cols[3][1] = from.b4; to.cols[3][2] = from.c4; to.cols[3][3] = from.d4;
        return to;
    }

    Model::Model(const std::string& path)
        : m_Directory(path.substr(0, path.find_last_of('/')))
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            GE_LOG_ERROR("Assimp: %s", importer.GetErrorString());
            return;
        }

        // Process meshes recursively
        auto processNode = [&](auto& self, aiNode* node, const aiScene* scene) -> void {
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;

                for (unsigned int v = 0; v < mesh->mNumVertices; v++)
                {
                    Vertex vertex;
                    vertex.Position[0] = mesh->mVertices[v].x;
                    vertex.Position[1] = mesh->mVertices[v].y;
                    vertex.Position[2] = mesh->mVertices[v].z;

                    if (mesh->HasNormals()) {
                        vertex.Normal[0] = mesh->mNormals[v].x;
                        vertex.Normal[1] = mesh->mNormals[v].y;
                        vertex.Normal[2] = mesh->mNormals[v].z;
                    }

                    if (mesh->mTextureCoords[0]) {
                        vertex.TexCoord[0] = mesh->mTextureCoords[0][v].x;
                        vertex.TexCoord[1] = mesh->mTextureCoords[0][v].y;
                    } else {
                        vertex.TexCoord[0] = 0.0f;
                        vertex.TexCoord[1] = 0.0f;
                    }

                    // Defaults
                    vertex.Color[0] = 1.0f; vertex.Color[1] = 1.0f; vertex.Color[2] = 1.0f; vertex.Color[3] = 1.0f;
                    vertex.EntityID = -1;
                    for(int b=0; b<4; b++) { vertex.BoneIDs[b] = -1; vertex.Weights[b] = 0.0f; }

                    vertices.push_back(vertex);
                }

                for (unsigned int f = 0; f < mesh->mNumFaces; f++)
                {
                    aiFace face = mesh->mFaces[f];
                    for (unsigned int j = 0; j < face.mNumIndices; j++)
                        indices.push_back(face.mIndices[j]);
                }

                // Extract Bone Weights
                for (unsigned int b = 0; b < mesh->mNumBones; b++)
                {
                    int boneID = -1;
                    std::string boneName = mesh->mBones[b]->mName.C_Str();
                    if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
                    {
                        ecs::AnimatorComponent::BoneInfo newBoneInfo;
                        newBoneInfo.id = m_BoneCounter;
                        newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[b]->mOffsetMatrix);
                        m_BoneInfoMap[boneName] = newBoneInfo;
                        boneID = m_BoneCounter;
                        m_BoneCounter++;
                    }
                    else
                    {
                        boneID = m_BoneInfoMap[boneName].id;
                    }

                    auto weights = mesh->mBones[b]->mWeights;
                    int numWeights = mesh->mBones[b]->mNumWeights;
                    for (int w = 0; w < numWeights; w++)
                    {
                        int vertexID = weights[w].mVertexId;
                        float weight = weights[w].mWeight;
                        for (int k = 0; k < 4; k++)
                        {
                            if (vertices[vertexID].BoneIDs[k] < 0)
                            {
                                vertices[vertexID].Weights[k] = weight;
                                vertices[vertexID].BoneIDs[k] = boneID;
                                break;
                            }
                        }
                    }
                }

                std::shared_ptr<Mesh> meshPtr = Mesh::Create(vertices, indices);
                m_Meshes.push_back({ meshPtr, mesh->mName.C_Str() });

                // Aggregate AABB
                const auto& meshAABB = meshPtr->GetAABB();
                m_AABB.Expand(meshAABB.Min);
                m_AABB.Expand(meshAABB.Max);
            }

            for (unsigned int i = 0; i < node->mNumChildren; i++)
                self(self, node->mChildren[i], scene);
        };

        processNode(processNode, scene->mRootNode, scene);

        // --- Extract Animations ---
        auto readHierarchyData = [&](auto& self, ecs::AnimatorComponent::SkeletalAnimation::Node& dest, const aiNode* src) -> void {
            dest.name = src->mName.data;
            dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
            for (unsigned int i = 0; i < src->mNumChildren; i++)
            {
                ecs::AnimatorComponent::SkeletalAnimation::Node child;
                self(self, child, src->mChildren[i]);
                dest.children.push_back(child);
            }
        };

        for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        {
            aiAnimation* anim = scene->mAnimations[i];
            ecs::AnimatorComponent::SkeletalAnimation sa;
            sa.duration = (float)anim->mDuration;
            sa.ticksPerSecond = (int)anim->mTicksPerSecond;
            sa.boneInfoMap = m_BoneInfoMap;
            readHierarchyData(readHierarchyData, sa.rootNode, scene->mRootNode);

            for (unsigned int j = 0; j < anim->mNumChannels; j++)
            {
                aiNodeAnim* channel = anim->mChannels[j];
                ecs::AnimatorComponent::BoneTrack track;
                track.name = channel->mNodeName.C_Str();

                for (unsigned int k = 0; k < channel->mNumPositionKeys; k++)
                {
                    sa.tracks[track.name].positions.push_back({
                        { channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z },
                        (float)channel->mPositionKeys[k].mTime
                    });
                }

                for (unsigned int k = 0; k < channel->mNumRotationKeys; k++)
                {
                    sa.tracks[track.name].rotations.push_back({
                        { channel->mRotationKeys[k].mValue.x, channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z, channel->mRotationKeys[k].mValue.w },
                        (float)channel->mRotationKeys[k].mTime
                    });
                }

                for (unsigned int k = 0; k < channel->mNumScalingKeys; k++)
                {
                    sa.tracks[track.name].scales.push_back({
                        { channel->mScalingKeys[k].mValue.x, channel->mScalingKeys[k].mValue.y, channel->mScalingKeys[k].mValue.z },
                        (float)channel->mScalingKeys[k].mTime
                    });
                }
            }
            m_Animations[anim->mName.C_Str()] = sa;
        }
    }

} // namespace renderer
} // namespace ge
