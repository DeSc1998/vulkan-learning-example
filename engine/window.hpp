#pragma once

#ifndef __linux__
#error Linux with X11 or Wayland is the only supported platform
#endif

#ifdef WAYLAND
#include <wayland-client-protocol-extra.hpp>
#include <wayland-client.hpp>

namespace wl = wayland;

#define VK_USE_PLATFORM_WAYLAND_KHR
#else
#include <X11/Xlib.h>

#define VK_USE_PLATFORM_XLIB_KHR
#endif

#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#include <vulkan/vulkan.hpp>

#include "utility.hpp"

namespace ds {

  static constexpr auto default_width  = 1200u;
  static constexpr auto default_height = 800u;

  class Window_handle final {
#ifdef WAYLAND
    wl::display_t     display;
    wl::event_queue_t event_queue;
    wl::registry_t    registry;
    wl::compositor_t  compositor;
    wl::surface_t     surface;
    wl::shell_t       shell;
    wl::shm_t         shm;
    wl::shm_pool_t    shm_pool;
    uint8_t*          pool_data = nullptr;

    std::array< wayland::buffer_t, 2 > buffers;

  private:
#else
    Display* display = nullptr;
    Window   window { };

    [[maybe_unused]] Atom delete_window = { };
#endif
  public:
    struct Size {
      size_t width = 0, height = 0;
    };

    struct Event {
      enum class Type {
        Nothing,
        Close_window,
        Client_message
      } type
        = Type::Nothing;
    };

    Window_handle( Size
                   = { .width = default_width, .height = default_height } );

    void set_window_name( std::string_view ) const;

    vk::Result init_surface( vk::Instance, vk::SurfaceKHR& ) const;

    vk::Extent2D get_extent( vk::PhysicalDevice, vk::SurfaceKHR ) const;
    bool         has_presentation_support( vk::PhysicalDevice, uint32_t ) const;
    Event        next_event( ) const;

    ~Window_handle( );
  };

}