
#include "pch.hpp"

#ifdef WAYLAND
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include "utility.hpp"
#include "window.hpp"

namespace ds {

#ifdef WAYLAND
  // assumed monitor dimesions
  static constexpr auto width = 1920u, height = 1080u;
  static constexpr auto stride                = width * sizeof( uint32_t );
  static constexpr auto default_shm_pool_size = height * stride * 2;

  static void random_filename( char* buf ) {
    auto current_time = std::chrono::high_resolution_clock::now( );
    long r            = current_time.time_since_epoch( ).count( );
    for ( int i = 0; i < 6; ++i ) {
      buf[i] = 'A' + ( r & 15 ) + ( r & 16 ) * 2;
      r >>= 5;
    }
  }

  static int create_shm_file( std::string& name ) {
    int retries = 100;
    name        = "/wl_shm-XXXXXX";
    do {
      random_filename( name.data( ) + name.size( ) - 7 );
      --retries;
      int fd = shm_open( name.c_str( ), O_RDWR | O_CREAT | O_EXCL,
                         S_IRUSR | S_IWUSR );
      if ( fd >= 0 ) {
        shm_unlink( name.c_str( ) );
        return fd;
      }
    } while ( retries > 0 && errno == EEXIST );
    return -1;
  }

  int allocate_shm_file( size_t size, std::string& name ) {
    int fd = create_shm_file( name );
    if ( fd < 0 )
      return -1;
    int ret;
    do {
      ret = ftruncate( fd, size );
    } while ( ret < 0 && errno == EINTR );
    if ( ret < 0 ) {
      close( fd );
      return -1;
    }
    return fd;
  }

  Window_handle::Window_handle( Window_handle::Size dim ) {
    (void)dim;

    event_queue = display.create_queue( );
    registry    = display.get_registry( );

    registry.on_global( )
      = [&]( uint32_t name, std::string interface, uint32_t version ) {
          if ( interface == wl::compositor_t::interface_name )
            registry.bind( name, compositor, version );
          // else if ( interface == wl::shell_t::interface_name )
          //   registry.bind( name, shell, version );
          // else if ( interface == wl::xdg_wm_base_t::interface_name )
          //   registry.bind( name, xdg_wm_base, version );
          // else if ( interface == wl::seat_t::interface_name )
          //   registry.bind( name, seat, version );
          else if ( interface == wl::shm_t::interface_name )
            registry.bind( name, shm, version );
        };

    display.roundtrip( );
    surface = compositor.create_surface( );

    // if ( xdg_wm_base ) {
    // } else {
    // }

    std::string filename;
    auto        fd = allocate_shm_file( default_shm_pool_size, filename );
    if ( fd < 0 ) {
      error( "failed to allocate shm file" );
      fmt::print( "    errno ({}): {}\n", errno, strerror( errno ) );
      exit( 1 );
    }

    pool_data = (uint8_t*)mmap( nullptr, default_shm_pool_size,
                                PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );

    if ( pool_data == MAP_FAILED ) {
      error( "failed to map pool data" );
    }

    shm_pool   = shm.create_pool( fd, default_shm_pool_size );
    buffers[0] = shm_pool.create_buffer( 0, width, height, stride,
                                         wl::shm_format::argb8888 );
    buffers[1] = shm_pool.create_buffer( height * stride, width, height, stride,
                                         wl::shm_format::argb8888 );

    const auto max_uint = std::numeric_limits< uint32_t >::max( );
    surface.attach( buffers[0], 0, 0 );
    surface.damage( 0, 0, max_uint, max_uint );
    surface.commit( );
  }

  void Window_handle::set_window_name( std::string_view name ) const {
    (void)name;
  }

  vk::Result Window_handle::init_surface( vk::Instance    instance,
                                          vk::SurfaceKHR& surf ) const {
    info( "creating surface" );

    vk::WaylandSurfaceCreateInfoKHR create_info { };

    create_info.display = display;
    create_info.surface = surface;

    return instance.createWaylandSurfaceKHR( &create_info, nullptr, &surf );
  }

  vk::Extent2D Window_handle::get_extent( vk::PhysicalDevice dev,
                                          vk::SurfaceKHR     surf ) const {
    vk::SurfaceCapabilitiesKHR capabilities { };
    (void)dev.getSurfaceCapabilitiesKHR(
      surf, &capabilities ); // TODO: assumed success

    auto extent = vk::Extent2D { };

    return vk::Extent2D {
      std::clamp( extent.width, capabilities.minImageExtent.width,
                  capabilities.maxImageExtent.width ),
      std::clamp( extent.height, capabilities.minImageExtent.height,
                  capabilities.maxImageExtent.height )
    };
  }

  Window_handle::Event Window_handle::next_event( ) const {
    auto event_count = display.dispatch_queue_pending( event_queue );
    (void)event_count;

    return { };
  }

  bool Window_handle::has_presentation_support( vk::PhysicalDevice dev,
                                                uint32_t index ) const {
    return dev.getWaylandPresentationSupportKHR( index, display );
  }

  Window_handle::~Window_handle( ) {
    munmap( pool_data, default_shm_pool_size );
  }
#else

  Window_handle::Window_handle( Window_handle::Size dim ) {
    display = XOpenDisplay( nullptr );
    if ( display == nullptr ) {
      error( "failed to open X11 Display" );
      exit( 1 );
    }

    auto screen = DefaultScreen( display );

    XSetWindowAttributes attr = { };
    attr.win_gravity          = CenterGravity;
    attr.background_pixel     = 0;
    unsigned long mask        = CWWinGravity | CWBackPixel;

    window = XCreateWindow( display, RootWindow( display, screen ), 0, 0,
                            dim.width, dim.height, 1, CopyFromParent,
                            InputOutput, CopyFromParent, mask, &attr );

    XSelectInput( display, window, ExposureMask | KeyPressMask );
    XMapWindow( display, window );

    delete_window = XInternAtom( display, "WM_DELETE_WINDOW", False );
    XSetWMProtocols( display, window, &delete_window, 1 );
  }

  void Window_handle::set_window_name( std::string_view name ) const {
    XStoreName( display, window, name.data( ) );
  }

  vk::Result Window_handle::init_surface( vk::Instance    instance,
                                          vk::SurfaceKHR& surf ) const {
    info( "creating surface" );

    vk::XlibSurfaceCreateInfoKHR create_info { };

    create_info.dpy    = display;
    create_info.window = window;

    return instance.createXlibSurfaceKHR( &create_info, nullptr, &surf );
  }

  vk::Extent2D Window_handle::get_extent( vk::PhysicalDevice dev,
                                          vk::SurfaceKHR     surf ) const {
    vk::SurfaceCapabilitiesKHR capabilities { };
    (void)dev.getSurfaceCapabilitiesKHR(
      surf, &capabilities ); // TODO: assumed success

    XWindowAttributes attr { };
    XGetWindowAttributes( display, window, &attr );
    auto extent = vk::Extent2D { static_cast< uint32_t >( attr.width ),
                                 static_cast< uint32_t >( attr.height ) };

    return vk::Extent2D {
      std::clamp( extent.width, capabilities.minImageExtent.width,
                  capabilities.maxImageExtent.width ),
      std::clamp( extent.height, capabilities.minImageExtent.height,
                  capabilities.maxImageExtent.height )
    };
  }

  static int match_event( Display*, XEvent* e, XPointer ) {
    if ( e->type == ClientMessage )
      return True;
    else
      return False;
  }

  Window_handle::Event Window_handle::next_event( ) const {
    XEvent event;

    if ( XCheckIfEvent( display, &event, &match_event, nullptr ) ) {
      if ( event.type == ClientMessage
           && (unsigned int)event.xclient.data.l[0] == delete_window )
        return { Event::Type::Close_window };
      else if ( event.type == ClientMessage )
        return { Event::Type::Client_message };
    }
    return { };
  }

  bool Window_handle::has_presentation_support( vk::PhysicalDevice,
                                                uint32_t ) const {
    return true;
  }

  Window_handle::~Window_handle( ) {
    if ( display != nullptr ) {
      XDestroyWindow( display, window );
      XCloseDisplay( display );

      display = nullptr;
      window  = 0;
    }
  }
#endif
}