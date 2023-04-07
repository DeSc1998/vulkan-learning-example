#pragma once

#ifndef __linux__
#error Linux with X11 or Wayland is the only supported platform
#endif

// TODO: explore wayland and xorg natively
// #ifdef WAYLAND
// #include <linux/input.h>
// #include <wayland-client-protocol-extra.hpp>
// #include <wayland-client.hpp>

// namespace wl = wayland;

// #define VK_USE_PLATFORM_WAYLAND_KHR
// #else
// #include <X11/Xlib.h>

// #define VK_USE_PLATFORM_XLIB_KHR
// #endif

#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VK_USE_PLATFORM_GLFW_KHR
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "utility.hpp"

namespace ds {

  static constexpr auto default_width  = 1200u;
  static constexpr auto default_height = 800u;

  class Window_handle final {
    GLFWwindow* window = nullptr;

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

    void set_window_name( std::string_view );

    std::vector< const char* > get_required_extensions( ) const;

    vk::Result init_surface( vk::Instance, vk::SurfaceKHR& ) const;

    vk::Extent2D get_extent( vk::PhysicalDevice, vk::SurfaceKHR ) const;
    bool         has_presentation_support( vk::Instance, vk::PhysicalDevice,
                                           uint32_t ) const;
    Event        next_event( ) const;

    ~Window_handle( );
  };

}