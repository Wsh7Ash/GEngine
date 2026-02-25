#pragma once

#include "../math/VecTypes.h"
#include "../math/Mat4x4.h"
#include <string>
#include <memory>

namespace ge {
namespace renderer {

    /**
     * @brief Interface for Shader programs.
     * Backends (GL, DX11) will implement the specific logic for compiling and setting uniforms.
     */
    class Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetInt(const std::string& name, int value) = 0;
        virtual void SetFloat(const std::string& name, float value) = 0;
        virtual void SetVec3(const std::string& name, const Math::Vec3f& value) = 0;
        virtual void SetVec4(const std::string& name, const Math::Vec4f& value) = 0;
        virtual void SetMat4(const std::string& name, const Math::Mat4f& value) = 0;

        /**
         * @brief Factory method to create a shader of the current API type.
         */
        static std::shared_ptr<Shader> Create(const std::string& vertexPath, const std::string& fragmentPath);
    };

} // namespace renderer
} // namespace ge
