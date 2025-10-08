#pragma once

#include "Framework/Common/glmCommon.hpp"
#include <vector>

#include "RenderContext.hpp"
#include "RenderFrame.hpp"
#include "Framework/Misc/BufferPool.hpp"
#include "Framework/Core/ShaderModule.hpp"
#include "Framework/Core/PipelineState.hpp"

namespace vkb
{
    class CommandBuffer;
    class RenderTarget;

    struct alignas(16) Light
    {
        glm::vec4 position;  // position.w represents type of light
        glm::vec4 color;     // color.w represents light intensity
        glm::vec4 direction; // direction.w represents range
        glm::vec2 info;
        // (only used for spot lights) info.x represents light inner cone angle, info.y represents light outer cone angle
    };

    struct LightingState
    {
        std::vector<Light> directional_lights;
        std::vector<Light> point_lights;
        std::vector<Light> spot_lights;
        BufferAllocation light_buffer;
    };

    /**
     * @brief Calculates the vulkan style projection matrix
     * @param proj The projection matrix
     * @return The vulkan style projection matrix
     */
    glm::mat4 vulkan_style_projection(const glm::mat4 &proj);

    // inline const std::vector<std::string> light_type_definitions = {
    //     "DIRECTIONAL_LIGHT " + std::to_string(static_cast<float>(sg::LightType::Directional)),
    //     "POINT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::Point)),
    //     "SPOT_LIGHT " + std::to_string(static_cast<float>(sg::LightType::Spot))};
    class Subpass
    {
    public:
        Subpass(vkb::RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader);

        Subpass(const Subpass &) = delete;
        Subpass &operator=(const Subpass &) = delete;

        Subpass(Subpass &&) = default;
        virtual ~Subpass() = default;

        Subpass &operator=(Subpass &&) = delete;

        /**
         * @brief Pure virtual function, used to record drawing commands into the specified command buffer.
         * @param command_buffer Command buffer used for recording drawing commands.
         */
        virtual void draw(vkb::CommandBuffer &command_buffer) = 0;

        /**
         * @brief Pure virtual function, used to prepare the shaders and shader variants required for the sub-channels.
         */
        virtual void prepare() = 0;

        /**
         * @brief Allocate and prepare the scene lighting data.
         * @code
         * vkb::LightingState lighting_state;
         * vkb::allocate_lightState(scene.get_components<sg::Light>(), MAX_DEFERRED_LIGHT_COUNT, lighting_state);
         * allocate_lights<DeferredLights>(lighting_state);
         * @endcode
         * @tparam  A lighting structure that contains members 'directional_lights', 'point_lights', and 'spot_light'.
         * @param   lighting_state The aggregation of lighting data requires an external conversion.
         */
        template <typename T>
        void allocate_lights(vkb::LightingState &lighting_state);

        // Getters
        const std::vector<uint32_t> &get_color_resolve_attachments() const;
        const std::string &get_debug_name() const;
        const uint32_t &get_depth_stencil_resolve_attachment() const;
        VkResolveModeFlagBits get_depth_stencil_resolve_mode() const;
        vkb::DepthStencilState &get_depth_stencil_state();
        const bool &get_disable_depth_stencil_attachment() const;
        const ShaderSource &get_fragment_shader() const;
        const std::vector<uint32_t> &get_input_attachments() const;
        vkb::LightingState &get_lighting_state();
        const std::vector<uint32_t> &get_output_attachments() const;
        vkb::RenderContext &get_render_context();
        const std::unordered_map<std::string, ShaderResourceMode> &get_resource_mode_map() const;
        VkSampleCountFlagBits get_sample_count() const;
        const ShaderSource &get_vertex_shader() const;

        // Setters
        void set_color_resolve_attachments(std::vector<uint32_t> const &color_resolve);
        void set_debug_name(const std::string &name);
        void set_disable_depth_stencil_attachment(bool disable_depth_stencil);
        void set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve);
        void set_depth_stencil_resolve_mode(VkResolveModeFlagBits mode);
        void set_input_attachments(std::vector<uint32_t> const &input);
        void set_output_attachments(std::vector<uint32_t> const &output);
        void set_sample_count(VkSampleCountFlagBits sample_count);

        /**
         * @brief Update the input and output attachment indices of the render target using the index of the attached file stored in this sub-channel.
         * This function is called by the RenderPipeline before starting the rendering channel and entering a new sub-channel.
         * @param render_target The render target that needs to be updated.
         */
        void update_render_target_attachments(vkb::RenderTarget &render_target);

    private:
        std::vector<uint32_t> color_resolve_attachments = {};

        std::string debug_name{};

        /**
         * @brief The parsing mode of the multi-sampling depth attachment.
         * If it is not VK_RESOLVE_MODE_NONE, the parsing of the depth/template attachment will be enabled.
         */
        VkResolveModeFlagBits depth_stencil_resolve_mode{VK_RESOLVE_MODE_NONE};

        vkb::DepthStencilState depth_stencil_state{};

        /**
         * @brief If it is true, then disable the depth/template attachments when creating the rendering channel.
         */
        bool disable_depth_stencil_attachment{false};

        uint32_t depth_stencil_resolve_attachment{VK_ATTACHMENT_UNUSED};

        // The structure that contains all the lighting data required for this sub-channel
        vkb::LightingState lighting_state{};

        std::vector<uint32_t> input_attachments = {};

        // Output the index list of the attachments, with the default being attachment 0 (which is usually the exchange chain image)
        std::vector<uint32_t> output_attachments = {0};

        vkb::RenderContext &render_context;

        std::unordered_map<std::string, ShaderResourceMode> resource_mode_map;

        VkSampleCountFlagBits sample_count{VK_SAMPLE_COUNT_1_BIT};

        ShaderSource vertex_shader;

        ShaderSource fragment_shader;
    };

    inline glm::mat4 vulkan_style_projection(const glm::mat4 &proj)
    {
        // Flip Y in clipspace. X = -1, Y = -1 is topLeft in Vulkan.
        glm::mat4 mat = proj;
        mat[1][1] *= -1;

        return mat;
    }

    template <typename T>
    void Subpass::allocate_lights(vkb::LightingState &lighting_state_in)
    {
        lighting_state.directional_lights.clear();
        lighting_state.point_lights.clear();
        lighting_state.spot_lights.clear();

        lighting_state.directional_lights = lighting_state_in.directional_lights;
        lighting_state.point_lights = lighting_state_in.point_lights;
        lighting_state.spot_lights = lighting_state_in.spot_lights;

        T light_info;

        std::copy(lighting_state.directional_lights.begin(), lighting_state.directional_lights.end(),
                  light_info.directional_lights);
        std::copy(lighting_state.point_lights.begin(), lighting_state.point_lights.end(), light_info.point_lights);
        std::copy(lighting_state.spot_lights.begin(), lighting_state.spot_lights.end(), light_info.spot_lights);

        auto &render_frame = render_context.get_active_frame();
        lighting_state.light_buffer = render_frame.allocate_buffer(
            VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(T));
        lighting_state.light_buffer.update(light_info);
    }
}
