#ifndef IMPL_H_
#define IMPL_H_

#include <array>
#include <filesystem>
#include <optional>
#include <vector>

#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VK_USE_PLATFORM_GLFW_KHR
#include <vulkan/vulkan.hpp>

#include "component.hpp"
#include "memory.hpp"
#include "window.hpp"

namespace ds {

  struct Camera {
    float position[4];
    float direction[4];
  };

  struct Uniform_buffer_data {
    vk::Buffer       buffer { };
    vk::DeviceMemory memory { };
    Camera*          mapped_memory = nullptr;
  };

  class Engine {
    Window_handle win;

    // VulkanHandler
    vk::Instance       instance { };
    vk::PhysicalDevice physical_device { };
    vk::Device         device { };
    vk::Queue          queue { };
    vk::SurfaceKHR     surface { };

    vk::SwapchainKHR swap_chain { };

    std::vector< vk::Image >     swap_chain_images { };
    std::vector< vk::ImageView > swap_chain_image_views { };

    vk::RenderPass render_pass { };

    vk::DescriptorSetLayout descriptor_layout { };
    vk::PipelineLayout      pipeline_layout { };
    vk::PipelineCache       pipeline_cache { };
    vk::Pipeline            graphics_pipeline { };

    std::vector< vk::Framebuffer > framebuffers { };

    vk::CommandPool                  command_pool { };
    std::vector< vk::CommandBuffer > command_buffer;

    std::vector< vk::Semaphore > sema_image_available;
    std::vector< vk::Semaphore > sema_render_finished;
    std::vector< vk::Fence >     fence_in_flight;

    // vk::Buffer                         index_buffer { };
    // vk::DeviceMemory                   index_buffer_memory { };
    std::vector< Uniform_buffer_data > uniforms;
    vk::DescriptorPool                 pool { };
    std::vector< vk::DescriptorSet >   sets;

    GPU_Memory< Component > memory;

    static std::vector< Component >           vertices;
    std::optional< std::vector< Component > > user_vertices;

    void create_instance( );
    void pick_physical_device( );
    void create_surface( );
    void create_device( );
    void create_swap_chain( );
    void create_image_views( );
    void create_descriptor_layout( );
    void create_graphics_pipeline( );
    void create_render_pass( );
    void create_framebuffers( );
    void create_command_pool( );
    void create_command_buffer( );
    void create_buffers( );
    void create_descriptor_pool( );
    void create_descriptor_sets( );
    void record_command_buffer( size_t index, size_t frame );
    void create_sync( );
    bool draw_frame( size_t frame );

    void recreate( );
    bool handle_result( vk::Result );

  public:
    Engine( ) = default;

    // TODO: to lazy to implement copy and move.
    //       might not be nessecary
    Engine( const Engine& )            = delete;
    Engine& operator=( const Engine& ) = delete;
    Engine( Engine&& )                 = delete;
    Engine& operator=( Engine&& )      = delete;

    void init_window( );
    void init_vulkan( );
    void loop( );

    void set_components( std::vector< Component > );

    ~Engine( );
  };

  struct queue_family_indicies {
    std::optional< uint32_t > graphics_family = { };
    std::optional< uint32_t > present_family  = { };

    inline bool is_complete( ) {
      return graphics_family.has_value( ) && present_family.has_value( );
    }

    inline std::vector< uint32_t > to_vec( ) {
      return { graphics_family.value( ), present_family.value( ) };
    }
  };

} // namespace ds

#endif // IMPL_H_
