#pragma once

#include <string>

#include "Engine/SceneGraph/Components/Camera.hpp"
#include "Framework/Common/VkError.hpp"
#include "Framework/Common/glmCommon.hpp"
#include "Framework/Common/VkHelpers.hpp"


namespace scene
{
    struct Cascade
    {
        float splitDepth;
        glm::mat4 viewProjMatrix;
    };

    /*(-1,-1)────────────────────────────(+1,-1)        
           │                              │               
           │                              │               
           │                              │     
           │    x ∈ [-1,+1]               │               
           │    y ∈ [-1,+1] ←─Y downward! │               
           │    z ∈ [ 0,+1]               │               
           │                              │               
     (-1,+1)──────────────────────────────(+1,+1)*/
    class PerspectiveCamera : public Camera
    {
    public:
        PerspectiveCamera();
        PerspectiveCamera(const std::string& name);

        PerspectiveCamera(const PerspectiveCamera&) = delete;
        PerspectiveCamera& operator=(const PerspectiveCamera&) = delete;

        PerspectiveCamera(PerspectiveCamera&& other) noexcept;
        PerspectiveCamera& operator=(PerspectiveCamera&& other) noexcept;

        virtual ~PerspectiveCamera() = default;

        void SetAspectRatio(float aspect_ratio);

        void SetFieldOfView(float fov);

        float GetFarPlane() const;

        void SetFarPlane(float zfar);

        float GetNearPlane() const;

        void SetNearPlane(float znear);

        float GetAspectRatio();

        float GetFieldOfView();

        const std::vector<Cascade>& GetCascades(uint32_t cascade_num, const glm::vec3& lightDir,
                                                float cascadeSplitLambda = 0.95f);

        const std::vector<Cascade>& GetCascades() const;

        virtual glm::mat4 GetProjection() override;

    private:
        /**
         * @brief Screen size aspect ratio
         */
        float aspect_ratio{1.0f};

        /**
         * @brief Horizontal field of view in radians
         */
        float fov{glm::radians(60.0f)};

        float far_plane{100.0};

        float near_plane{0.1f};

        mutable std::vector<Cascade> Cascades;
    };
}
