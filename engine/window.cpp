
#include "pch.hpp"

#include "utility.hpp"
#include "window.hpp"

namespace ds {

  Window_handle::Window_handle( Window_handle::Size dim ) {
    if ( glfwInit( ) != GLFW_TRUE ) {
      error( "failed to initilaize glfw" );
      exit( 1 );
    }

    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    window
      = glfwCreateWindow( dim.width, dim.height, "unnamed", nullptr, nullptr );

    if ( window == nullptr ) { }

    glfwMakeContextCurrent( window );
  }

  void Window_handle::set_window_name( std::string_view name ) {
    glfwSetWindowTitle( window, name.data( ) );
  }

  std::vector< const char* > Window_handle::get_required_extensions( ) const {
    uint32_t     count = 0;
    const char** exts  = glfwGetRequiredInstanceExtensions( &count );
    if ( count > 0 ) {
      // info( "listing extensions" );
      // for ( uint32_t i = 0; i < count; ++i ) {
      //   info( exts[i] );
      // }
      return std::vector< const char* > { exts, exts + count };
    } else
      return { };
  }

  vk::Result Window_handle::init_surface( vk::Instance    instance,
                                          vk::SurfaceKHR& surf ) const {
    VkSurfaceKHR s = { };
    auto result    = glfwCreateWindowSurface( instance, window, nullptr, &s );
    if ( result == VK_SUCCESS ) {
      surf = s;
    }
    return vk::Result { result };
  }

  vk::Extent2D Window_handle::get_extent( vk::PhysicalDevice dev,
                                          vk::SurfaceKHR     surf ) const {
    vk::SurfaceCapabilitiesKHR capabilities { };
    (void)dev.getSurfaceCapabilitiesKHR(
      surf, &capabilities ); // TODO: assumed success

    int width, height;
    glfwGetWindowSize( window, &width, &height );
    auto extent = vk::Extent2D { static_cast< uint32_t >( width ),
                                 static_cast< uint32_t >( height ) };

    return vk::Extent2D {
      std::clamp( extent.width, capabilities.minImageExtent.width,
                  capabilities.maxImageExtent.width ),
      std::clamp( extent.height, capabilities.minImageExtent.height,
                  capabilities.maxImageExtent.height )
    };
  }

  Window_handle::Event Window_handle::next_event( ) const {
    if ( glfwWindowShouldClose( window ) ) {
      return { Event::Type::Close_window };
    }

    glfwPollEvents( );

    // TODO: handle incomming events

    return { };
  }

  bool Window_handle::has_presentation_support( vk::Instance       instance,
                                                vk::PhysicalDevice dev,
                                                uint32_t index ) const {
    return glfwGetPhysicalDevicePresentationSupport( instance, dev, index )
           == GLFW_TRUE;
  }

  Window_handle::~Window_handle( ) { glfwTerminate( ); }

}