#ifndef IMPL_H_
#define IMPL_H_

#include <filesystem>
#include <optional>
#include <vector>

#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VK_USE_PLATFORM_GLFW_KHR
#include <vulkan/vulkan.hpp>

#include "memory.hpp"
#include "window.hpp"

namespace ds {

  struct Component;

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

    vk::PipelineLayout pipeline_layout { };
    vk::PipelineCache  pipeline_cache { };
    vk::Pipeline       graphics_pipeline { };

    std::vector< vk::Framebuffer > framebuffers { };

    vk::CommandPool   command_pool { };
    vk::CommandBuffer command_buffer { };

    vk::Semaphore sema_image_available { };
    vk::Semaphore sema_render_finished { };
    vk::Fence     fence_in_flight { };

    GPU_Memory< Component > memory;

    static std::vector< Component > vertices;

    void create_instance( );
    void pick_physical_device( );
    void create_surface( );
    void create_device( );
    void create_swap_chain( );
    void create_image_views( );
    void create_graphics_pipeline( );
    void create_render_pass( );
    void create_framebuffers( );
    void create_command_pool( );
    void create_command_buffer( );
    void create_buffers( );
    void record_command_buffer( size_t image_index );
    void create_sync( );
    void draw_frame( );

    void recreate( );
    void handle_result( vk::Result );

  public:
    Engine( ) = default;

    // TODO: to lazy to implement copy and move
    //       might not be nessecary
    Engine( const Engine& )            = delete;
    Engine& operator=( const Engine& ) = delete;
    Engine( Engine&& )                 = delete;
    Engine& operator=( Engine&& )      = delete;

    void init_window( );
    void init_vulkan( );
    void loop( );

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
