
#include "pch.hpp"

#include "component.hpp"
#include "impl.hpp"
#include "utility.hpp"

namespace ds {

  [[maybe_unused]] static constexpr auto screen_width  = 1200;
  [[maybe_unused]] static constexpr auto screen_height = 800;

  static const std::vector< const char* > validation_layers
    = { "VK_LAYER_KHRONOS_validation" };
#ifndef WAYLAND
  static const std::vector< const char* > instance_extensions
    = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
#else
  static const std::vector< const char* > instance_extensions
    = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
#endif

  static const std::vector< const char* > device_extensions
    = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME };

  static queue_family_indicies indicies;

#define Unused( x ) (void)x

#define Fill( dev, prop, func, dest )      \
  (void)dev.func( prop, &count, nullptr ); \
  dest.resize( count );                    \
  (void)dev.func( prop, &count, dest.data( ) )

  static constexpr auto enable_validation_layer = debug_mode;

  bool check_validation_layer_support( ) {
    std::vector< vk::LayerProperties > available_layers;
    uint32_t                           count;
    (void)vk::enumerateInstanceLayerProperties( &count, nullptr );
    available_layers.resize( count );
    (void)vk::enumerateInstanceLayerProperties( &count,
                                                available_layers.data( ) );

    if constexpr ( debug_mode )
      for ( const auto& layer_prop : available_layers ) {
        fmt::print( "INFO: found layer '{}'\n", layer_prop.layerName.data( ) );
      }

    for ( const std::string_view layer_name : validation_layers ) {
      auto layer_found = false;

      if constexpr ( debug_mode )
        fmt::print( "INFO: checking for layer '{}'\n", layer_name.data( ) );

      for ( const auto& layer_properties : available_layers ) {
        if ( layer_name == layer_properties.layerName ) {
          layer_found = true;
          break;
        }
      }

      if ( !layer_found ) {
        fmt::print( "ERROR: layer not found\n" );
        return false;
      }
    }

    return true;
  }

  std::vector< const char* > get_required_extentions( ) {
    std::vector< const char* > extentions { };

    if constexpr ( enable_validation_layer ) {
      extentions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    for ( auto ext : instance_extensions ) {
      extentions.push_back( ext );
    }

    return extentions;
  }

  unsigned int
    debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT      msg_severity,
                    VkDebugUtilsMessageTypeFlagsEXT             msg_type,
                    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                    void*                                       user_data ) {
    Unused( msg_type );
    Unused( user_data );

    if ( callback_data != nullptr && callback_data->pMessage != nullptr ) {
      if ( msg_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) {
        fprintf( stderr, "ERROR: %s\n", callback_data->pMessage );
      } else if ( msg_severity
                  >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) {
        fprintf( stdout, "WARNING: %s\n", callback_data->pMessage );
      } else if ( msg_severity
                  >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ) {
        fprintf( stdout, "VERBOSE: %s\n", callback_data->pMessage );
      }
    }

    return VK_TRUE;
  }

  void Engine::init_window( ) { win.set_window_name( "triangle" ); }

  void Engine::create_instance( ) {
    vk::ApplicationInfo app_info { };

    vk::InstanceCreateInfo create_info { };

    if constexpr ( enable_validation_layer ) {
      if ( !check_validation_layer_support( ) ) {
        fmt::print(
          "ERROR: validation layers were requested, but are not available\n" );
        exit( 1 );
      }
    }

    app_info.pApplicationName   = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION( 0, 1, 0 );
    app_info.pEngineName        = "Not Defined";
    app_info.engineVersion      = VK_MAKE_VERSION( 0, 1, 0 );
    app_info.apiVersion         = VK_API_VERSION_1_3;

    auto extentions = get_required_extentions( );

    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = extentions.size( );
    create_info.ppEnabledExtensionNames = extentions.data( );

    [[maybe_unused]] vk::DebugUtilsMessengerCreateInfoEXT debug_info { };
    if constexpr ( enable_validation_layer ) {
      debug_info.messageSeverity
        = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
          | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
          | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
      debug_info.messageType
        = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
          | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
          | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
      debug_info.pfnUserCallback = &debug_callback;

      create_info.pNext               = &debug_info;
      create_info.enabledLayerCount   = validation_layers.size( );
      create_info.ppEnabledLayerNames = validation_layers.data( );
    }

    vk::Result result = vk::createInstance( &create_info, nullptr, &instance );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to create vulkan instance", result );
    }
  }

  queue_family_indicies& find_queue_families( vk::PhysicalDevice device,
                                              vk::SurfaceKHR&    surface ) {
    if ( indicies.is_complete( ) ) {
      return indicies;
    }

    std::vector< vk::QueueFamilyProperties > queue_props;
    uint32_t                                 count;
    device.getQueueFamilyProperties( &count, nullptr );
    queue_props.resize( count );
    device.getQueueFamilyProperties( &count, queue_props.data( ) );

    if constexpr ( debug_mode )
      fmt::print( "INFO: queue family count = {}\n", queue_props.size( ) );

    for ( size_t i = 0; i < queue_props.size( ); ++i ) {
      if ( queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics ) {
        indicies.graphics_family = i;

        uint32_t support = 1;
        if ( device.getSurfaceSupportKHR( i, surface, &support )
               == vk::Result::eSuccess
             && support ) {
          indicies.present_family = i;
        }

        if constexpr ( debug_mode )
          fmt::print( "INFO: family index: {}\n", i );

        break;
      }
    }

    return indicies;
  }

  void Engine::pick_physical_device( ) {
    auto has_extension_support
      = []( const vk::PhysicalDevice& device ) -> bool {
      std::vector< vk::ExtensionProperties > available_extensions { };
      uint32_t                               count;
      (void)device.enumerateDeviceExtensionProperties( nullptr, &count,
                                                       nullptr );
      auto size = available_extensions.size( );
      available_extensions.resize( count + available_extensions.size( ) );
      (void)device.enumerateDeviceExtensionProperties(
        nullptr, &count, available_extensions.data( ) + size );

      std::set< std::string_view > required_extensions {
        device_extensions.begin( ), device_extensions.end( )
      };

      for ( const auto& extension : available_extensions ) {
        std::string_view name = extension.extensionName;
        // if constexpr ( debug_mode )
        //   fmt::print( "INFO: found extension: '{}'\n", name );

        required_extensions.erase( name );
      }

      if ( !required_extensions.empty( ) && debug_mode ) {
        for ( auto& ext : required_extensions ) {
          fmt::print( "INFO: extension not found: '{}'\n", ext );
        }
      }

      return required_extensions.empty( );
    };

    auto is_suitable = [&]( const vk::PhysicalDevice dev ) -> bool {
      vk::PhysicalDeviceProperties device_properties;
      vk::PhysicalDeviceFeatures   device_features;
      dev.getProperties( &device_properties );
      dev.getFeatures( &device_features );
      auto indicies            = find_queue_families( dev, surface );
      auto swap_chain_adequate = false;
      auto present_support     = win.has_presentation_support(
        dev, indicies.graphics_family.value( ) );

      if ( has_extension_support( dev ) ) {
        std::vector< vk::SurfaceFormatKHR > formats { };
        std::vector< vk::PresentModeKHR >   modes { };
        uint32_t                            count = 0;
        Fill( dev, surface, getSurfaceFormatsKHR, formats );
        Fill( dev, surface, getSurfacePresentModesKHR, modes );
        swap_chain_adequate = !formats.empty( ) && !modes.empty( );
      }

      return device_properties.deviceType
               == vk::PhysicalDeviceType::eDiscreteGpu
             && device_features.geometryShader && indicies.is_complete( )
             && swap_chain_adequate && present_support;
    };

    std::vector< vk::PhysicalDevice > devices;
    uint32_t                          count;
    (void)instance.enumeratePhysicalDevices( &count, nullptr );
    devices.resize( count );
    (void)instance.enumeratePhysicalDevices( &count, devices.data( ) );

    if ( devices.size( ) < 1 ) {
      error( "could not find a GPU with Vulkan support" );
    } else {
      for ( const auto& dev : devices ) {
        vk::PhysicalDeviceProperties prop;
        dev.getProperties( &prop );

        if constexpr ( debug_mode )
          fmt::print( "INFO: checking device '{}'\n", prop.deviceName.data( ) );

        if ( is_suitable( dev ) ) {
          physical_device = dev;
          break;
        }
      }

      if ( !physical_device ) {
        error( "could not find suitable GPU" );
        exit( 1 );
      }

      vk::PhysicalDeviceProperties props;
      physical_device.getProperties( &props );

      fmt::print( "INFO: chosen device '{}'\n", props.deviceName.data( ) );
    }
  }

  void Engine::create_device( ) {
    if constexpr ( debug_mode )
      fmt::print( "INFO: creating device\n" );

    const auto& indicies = find_queue_families( physical_device, surface );

    std::vector< vk::DeviceQueueCreateInfo > queue_infos { };
    std::set< uint32_t >                     unique_families
      = { indicies.graphics_family.value( ), indicies.present_family.value( ) };
    float                      priority    = 1.0f;
    vk::DeviceCreateInfo       device_info = { };
    vk::PhysicalDeviceFeatures device_features;
    physical_device.getFeatures( &device_features );

    for ( auto queue_family : unique_families ) {
      vk::DeviceQueueCreateInfo queue_info = { };

      queue_info.queueFamilyIndex = queue_family;
      queue_info.queueCount       = 1;
      queue_info.pQueuePriorities = &priority;

      queue_infos.push_back( queue_info );
    }

    device_info.queueCreateInfoCount = queue_infos.size( );
    device_info.pQueueCreateInfos    = queue_infos.data( );
    device_info.pEnabledFeatures     = &device_features;
    device_info.enabledExtensionCount
      = static_cast< uint32_t >( device_extensions.size( ) );
    device_info.ppEnabledExtensionNames = device_extensions.data( );

    if constexpr ( enable_validation_layer ) {
      device_info.enabledLayerCount   = validation_layers.size( );
      device_info.ppEnabledLayerNames = validation_layers.data( );
    } else {
      device_info.enabledLayerCount = 0;
    }

    auto result
      = physical_device.createDevice( &device_info, nullptr, &device );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to create device", result );
    }

    device.getQueue( indicies.present_family.value_or( 0 ), 0, &queue );
    if constexpr ( debug_mode )
      fmt::print( "INFO: finished creating device\n" );
  }

  void Engine::create_surface( ) {
    auto result = win.init_surface( instance, surface );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to create Surface", result );
      exit( 1 );
    }
    if constexpr ( debug_mode )
      fmt::print( "INFO: finished creating surface\n" );
  }

  vk::SurfaceFormatKHR choose_surface_format( vk::PhysicalDevice& p,
                                              vk::SurfaceKHR&     s ) {
    std::vector< vk::SurfaceFormatKHR > formats;
    uint32_t                            count = 0;
    Fill( p, s, getSurfaceFormatsKHR, formats );
    // uint32_t                            count;
    // (void)p.getSurfaceFormatsKHR( s, &count, nullptr );
    // formats.resize( count );
    // (void)p.getSurfaceFormatsKHR( s, &count, formats.data( ) );

    auto format = std::find_if(
      formats.begin( ), formats.end( ), []( const vk::SurfaceFormatKHR& f ) {
        return f.format == vk::Format::eB8G8R8A8Srgb
               && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
      } );

    return format != formats.end( ) ? *format : *formats.begin( );
  }

  void Engine::create_swap_chain( ) {
    if constexpr ( debug_mode )
      fmt::print( "INFO: creating swap chain\n" );

    auto choose_present_mode =
      [](
        const std::vector< vk::PresentModeKHR >& modes ) -> vk::PresentModeKHR {
      if constexpr ( debug_mode ) {
        fmt::print( "INFO: chosing present mode\n" );
        fmt::print( "INFO: number of modes: {}\n", modes.size( ) );
      }

      auto mode = std::find_if( modes.begin( ), modes.end( ),
                                []( const vk::PresentModeKHR& mode ) {
                                  return mode == vk::PresentModeKHR::eMailbox;
                                } );

      if constexpr ( debug_mode )
        fmt::print( "INFO: chosen mode at: {}\n", mode - modes.begin( ) );
#ifdef WAYLAND
      return vk::PresentModeKHR::eMailbox;
#else
      return mode != modes.end( ) ? *mode : vk::PresentModeKHR::eFifo;
#endif
    };

    vk::SurfaceCapabilitiesKHR capabiliteies { };
    (void)physical_device.getSurfaceCapabilitiesKHR( surface, &capabiliteies );

    uint32_t count;
    auto&&   surface_format = choose_surface_format( physical_device, surface );
    std::vector< vk::PresentModeKHR > modes { };
    auto&&                            mode = choose_present_mode( modes );

    (void)physical_device.getSurfacePresentModesKHR( surface, &count, nullptr );
    modes.resize( count );
    (void)physical_device.getSurfacePresentModesKHR( surface, &count,
                                                     modes.data( ) );

    const auto extent          = win.get_extent( physical_device, surface );
    const auto max_image_count = capabiliteies.maxImageCount;
    const auto min_image_count = capabiliteies.minImageCount;
    const auto image_count
      = max_image_count > 0 && min_image_count > max_image_count
          ? max_image_count
          : min_image_count + 1;

    vk::SwapchainCreateInfoKHR create_info { };

    info( "filling creation info" );

    create_info.minImageCount    = image_count;
    create_info.surface          = surface;
    create_info.imageFormat      = surface_format.format;
    create_info.imageExtent      = extent;
    create_info.presentMode      = mode;
    create_info.clipped          = vk::Bool32( true );
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
    create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.preTransform
      = capabiliteies.currentTransform; // no special transform

    const auto& indicies = find_queue_families( physical_device, surface );
    uint32_t    queue_indicies[2];

    queue_indicies[0] = indicies.graphics_family.value( );
    queue_indicies[1] = indicies.present_family.value( );

    if ( queue_indicies[0] != queue_indicies[1] ) {
      create_info.imageSharingMode      = vk::SharingMode::eConcurrent;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices   = queue_indicies;
    } else {
      create_info.imageSharingMode = vk::SharingMode::eExclusive;
    }

    if ( swap_chain ) {
      create_info.oldSwapchain = swap_chain;
    }

    auto result
      = device.createSwapchainKHR( &create_info, nullptr, &swap_chain );

    exit_on_fail( "failed to create Swapchain", result );

    if ( create_info.oldSwapchain )
      device.destroy( create_info.oldSwapchain, nullptr );

    if constexpr ( debug_mode )
      fmt::print( "INFO: finished creating swap chain\n" );

    (void)device.getSwapchainImagesKHR( swap_chain, &count, nullptr );

    swap_chain_images.resize( count );
    result = device.getSwapchainImagesKHR( swap_chain, &count,
                                           swap_chain_images.data( ) );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to retrive images", result );
    }
  }

  void Engine::create_image_views( ) {
    swap_chain_image_views.reserve( swap_chain_images.size( ) );

    for ( auto& image : swap_chain_images ) {
      vk::ImageViewCreateInfo create_info { };
      vk::ImageView           tmp { };

      create_info.image = image;

      create_info.viewType = vk::ImageViewType::e2D;
      auto f               = choose_surface_format( physical_device, surface );
      create_info.format   = f.format;

      create_info.components.a = vk::ComponentSwizzle::eIdentity;
      create_info.components.g = vk::ComponentSwizzle::eIdentity;
      create_info.components.b = vk::ComponentSwizzle::eIdentity;
      create_info.components.r = vk::ComponentSwizzle::eIdentity;

      create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      create_info.subresourceRange.baseMipLevel   = 0;
      create_info.subresourceRange.levelCount     = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount     = 1;

      auto result = device.createImageView( &create_info, nullptr, &tmp );

      if ( result != vk::Result::eSuccess ) {
        error( "failed to create ImageView", result );
        continue;
      }

      swap_chain_image_views.emplace_back( std::move( tmp ) );
    }
  }

  void Engine::create_render_pass( ) {
    vk::RenderPassCreateInfo  r { };
    vk::AttachmentDescription colAtt { };
    vk::AttachmentReference   colRef { };
    vk::SubpassDescription    sub { };
    vk::SubpassDependency     dep { };

    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dep.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dep.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    colAtt.format  = choose_surface_format( physical_device, surface ).format;
    colAtt.samples = vk::SampleCountFlagBits::e1;
    colAtt.loadOp  = vk::AttachmentLoadOp::eClear;
    colAtt.storeOp = vk::AttachmentStoreOp::eStore;
    colAtt.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    colAtt.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colAtt.initialLayout  = vk::ImageLayout::eUndefined;
    colAtt.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

    colRef.attachment = 0;
    colRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;

    sub.colorAttachmentCount = 1;
    sub.pColorAttachments    = &colRef;

    r.attachmentCount = 1;
    r.pAttachments    = &colAtt;
    r.subpassCount    = 1;
    r.pSubpasses      = &sub;
    r.dependencyCount = 1;
    r.pDependencies   = &dep;

    auto result = device.createRenderPass( &r, nullptr, &render_pass );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to create RenderPass", result );
    }
  }

  vk::ShaderModule create_shader_module( vk::Device&        dev,
                                         const std::string& code ) {
    vk::ShaderModuleCreateInfo create_info { };
    vk::ShaderModule           tmp { };

    create_info.codeSize = code.size( );
    create_info.pCode    = reinterpret_cast< const uint32_t* >( code.data( ) );

    auto result = dev.createShaderModule( &create_info, nullptr, &tmp );
    if ( result != vk::Result::eSuccess ) {
      error( "failed to create ShaderModule", result );
    }

    return tmp;
  }

  void Engine::create_graphics_pipeline( ) {
    // TODO: extract shader setup
    const auto vertex_code   = read_file( "build/vert.spv" );
    const auto fragment_code = read_file( "build/frag.spv" );

    auto vertex_module   = create_shader_module( device, vertex_code );
    auto fragment_module = create_shader_module( device, fragment_code );

    vk::PipelineShaderStageCreateInfo create_info_vert { };
    create_info_vert.stage  = vk::ShaderStageFlagBits::eVertex;
    create_info_vert.module = vertex_module;
    create_info_vert.pName  = "main";

    vk::PipelineShaderStageCreateInfo create_info_frag { };
    create_info_frag.stage  = vk::ShaderStageFlagBits::eFragment;
    create_info_frag.module = fragment_module;
    create_info_frag.pName  = "main";

    vk::PipelineShaderStageCreateInfo shader_stages[]
      = { create_info_vert, create_info_frag };

    std::vector< vk::DynamicState > dynamic_states
      = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    auto swap_chain_extent = win.get_extent( physical_device, surface );

    vk::Viewport viewport { };
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)swap_chain_extent.width;
    viewport.height   = (float)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor { };
    scissor.extent = swap_chain_extent;

    vk::PipelineDynamicStateCreateInfo dynamic_state { };
    dynamic_state.dynamicStateCount
      = static_cast< uint32_t >( dynamic_states.size( ) );
    dynamic_state.pDynamicStates = dynamic_states.data( );

    vk::PipelineVertexInputStateCreateInfo vertex_input_info { };
    auto vertex_input_binding = ds::Component::binding_description( );
    auto vertex_input_desc    = ds::Component::attribute_description( );

    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions    = &vertex_input_binding;
    vertex_input_info.vertexAttributeDescriptionCount
      = vertex_input_desc.size( );
    vertex_input_info.pVertexAttributeDescriptions = vertex_input_desc.data( );

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly { };
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    vk::PipelineViewportStateCreateInfo viewportState { };
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer { };
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth   = 1.0f;
    rasterizer.cullMode    = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace   = vk::FrontFace::eClockwise;

    vk::PipelineMultisampleStateCreateInfo multisampling { };
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment { };
    colorBlendAttachment.colorWriteMask
      = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eR;

    vk::PipelineColorBlendStateCreateInfo colorBlending { };
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    vk::PipelineLayoutCreateInfo p { };

    auto result = device.createPipelineLayout( &p, nullptr, &pipeline_layout );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to create pipeline layout", result );
    }

    vk::PipelineCacheCreateInfo cache_info { };

    result
      = device.createPipelineCache( &cache_info, nullptr, &pipeline_cache );

    vk::GraphicsPipelineCreateInfo gp { };

    gp.stageCount          = 2;
    gp.pStages             = shader_stages;
    gp.pVertexInputState   = &vertex_input_info;
    gp.pInputAssemblyState = &inputAssembly;
    gp.pViewportState      = &viewportState;
    gp.pRasterizationState = &rasterizer;
    gp.layout              = pipeline_layout;
    gp.pMultisampleState   = &multisampling;
    gp.renderPass          = render_pass;
    gp.pColorBlendState    = &colorBlending;
    gp.pDynamicState       = &dynamic_state;

    result = device.createGraphicsPipelines( pipeline_cache, 1, &gp, nullptr,
                                             &graphics_pipeline );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to create pipeline", result );
    }

    device.destroy( vertex_module, nullptr );
    device.destroy( fragment_module, nullptr );
  }

  void free_framebuffers( vk::Device&                     device,
                          std::vector< vk::Framebuffer >& frames ) {
    for ( auto& framebuffer : frames ) {
      device.destroyFramebuffer( framebuffer, nullptr );
    }

    frames.clear( );
  }

  void Engine::create_framebuffers( ) {
    if ( framebuffers.size( ) != 0 )
      free_framebuffers( device, framebuffers );

    framebuffers.reserve( swap_chain_image_views.size( ) );
    auto extent = win.get_extent( physical_device, surface );

    for ( auto& view : swap_chain_image_views ) {
      vk::FramebufferCreateInfo b { };
      vk::Framebuffer           tmp { };

      b.renderPass      = render_pass;
      b.attachmentCount = 1;
      b.pAttachments    = &view;
      b.width           = extent.width;
      b.height          = extent.height;
      b.layers          = 1;

      auto result = device.createFramebuffer( &b, nullptr, &tmp );

      if ( result != vk::Result::eSuccess ) {
        error( "failed to create framebuffer", result );
        continue;
      }

      framebuffers.emplace_back( std::move( tmp ) );
    }
  }

  void Engine::create_command_pool( ) {
    auto indecies = find_queue_families( physical_device, surface );
    vk::CommandPoolCreateInfo c { };

    c.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    c.queueFamilyIndex = indecies.graphics_family.value( );

    auto result = device.createCommandPool( &c, nullptr, &command_pool );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to create CommandPool", result );
    }
  }

  std::vector< Component > Engine::vertices = {
    { { 0.3, -0.4, 0.5, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, 0.0 },
    { { 0.0, 0.4, 0.5, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, 0.0 },
    { { -0.3, -0.4, 0.5, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, 0.0 },
  };

  void Engine::create_buffers( ) {
    memory
      = GPU_Memory< Component > { device, physical_device, vertices.size( ) };

    size_t i = 0;
    for ( auto& com : vertices ) {
      memory[i++] = com;
    }
  }

  void Engine::record_command_buffer( size_t image_index ) {
    vk::CommandBufferBeginInfo cb_info { };
    vk::RenderPassBeginInfo    rp_info { };
    vk::ClearValue clear_color { std::array< float, 4 > { 0.1, 0.1, 0.1,
                                                          0.2 } };
    vk::Viewport   view { };
    vk::Rect2D     scissor { };
    auto           extent = win.get_extent( physical_device, surface );

    view.x        = 0.0;
    view.y        = 0.0;
    view.width    = extent.width;
    view.height   = extent.height;
    view.minDepth = 0.0;
    view.maxDepth = 1.0;

    scissor.extent = extent;
    // scissor.offset = vk::Offset2D { 0, 0 };

    rp_info.renderPass  = render_pass;
    rp_info.framebuffer = framebuffers[image_index];
    // rp_info.renderArea.offset = vk::Offset2D { 0, 0 };
    rp_info.renderArea.extent = extent;
    rp_info.clearValueCount   = 1;
    rp_info.pClearValues      = &clear_color;

    cb_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk::DeviceSize offsets[] = { 0 };

    (void)command_buffer.begin( &cb_info );
    command_buffer.bindVertexBuffers( 0, 1, memory.expose_buffer( ), offsets );
    command_buffer.beginRenderPass( &rp_info, vk::SubpassContents::eInline );
    command_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics,
                                 graphics_pipeline );
    command_buffer.setViewport( 0, 1, &view );
    command_buffer.setScissor( 0, 1, &scissor );
    command_buffer.draw( vertices.size( ), 1, 0, 0 );
    command_buffer.endRenderPass( );
    auto result = command_buffer.end( );

    if ( result != vk::Result::eSuccess ) {
      error( "an error accured during recording", result );
    }
  }

  void Engine::create_command_buffer( ) {
    vk::CommandBufferAllocateInfo cba { };

    cba.commandPool        = command_pool;
    cba.level              = vk::CommandBufferLevel::ePrimary;
    cba.commandBufferCount = 1;

    auto result = device.allocateCommandBuffers( &cba, &command_buffer );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to allocate command buffers", result );
    }
  }

  void Engine::create_sync( ) {
    vk::SemaphoreCreateInfo s { };
    vk::FenceCreateInfo     f { };

    f.flags = vk::FenceCreateFlagBits::eSignaled;

    (void)device.createSemaphore( &s, nullptr, &sema_image_available );
    (void)device.createSemaphore( &s, nullptr, &sema_render_finished );
    (void)device.createFence( &f, nullptr, &fence_in_flight );
  }

  void Engine::init_vulkan( ) {
    create_instance( );
    create_surface( );
    pick_physical_device( );
    create_device( );
    create_swap_chain( );
    create_image_views( );
    create_render_pass( );
    create_graphics_pipeline( );
    create_framebuffers( );
    create_command_pool( );
    create_command_buffer( );
    create_buffers( );
    create_sync( );
  }

  [[nodiscard]] bool recover_device( vk::Device& dev ) {
    // TODO: recover device
    Unused( dev );

    return false;
  }

  void Engine::recreate( ) {
    (void)device.waitIdle( );

    if constexpr ( debug_mode )
      info( "freeing image views" );
    for ( auto& image_view : swap_chain_image_views ) {
      device.destroyImageView( image_view, nullptr );
    }
    swap_chain_image_views.clear( );

    create_swap_chain( );
    create_image_views( );
    create_framebuffers( );

    (void)device.resetFences( 1, &fence_in_flight );
  }

  void Engine::handle_result( vk::Result result ) {
    if ( result == vk::Result::eErrorOutOfDateKHR ) {
      if constexpr ( debug_mode )
        info( "out of date: recreating swap chain and framebuffers" );

      recreate( );

      if constexpr ( debug_mode )
        info( "finished recreating" );
    } else if ( result == vk::Result::eErrorDeviceLost ) {
      if constexpr ( debug_mode )
        info( "device has been lost" );

      if ( !recover_device( device ) ) {
        error( "failed to recover device", vk::Result::eErrorDeviceLost );
        exit( 1 );
      }
    } else if ( result == vk::Result::eTimeout ) {
      if constexpr ( debug_mode )
        info( "an action has been timed out" );
    } else if ( result != vk::Result::eSuccess ) {
      error( "an error accured", result );
    }
  }

  void Engine::draw_frame( ) {
    auto result = device.waitForFences( 1, &fence_in_flight, vk::Bool32( true ),
                                        10'000'000 );

    handle_result( result );

    result = device.resetFences( 1, &fence_in_flight );
    handle_result( result );

    uint32_t index { };
    result = device.acquireNextImageKHR(
      swap_chain, std::numeric_limits< uint64_t >::max( ), sema_image_available,
      nullptr, &index );

    handle_result( result );

    result = command_buffer.reset( vk::CommandBufferResetFlags { 0 } );
    handle_result( result );

    record_command_buffer( index );

    vk::SubmitInfo         submit { };
    vk::PipelineStageFlags flags[]
      = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &sema_image_available;
    submit.pWaitDstStageMask    = flags;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &command_buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &sema_render_finished;

    result = queue.submit( 1, &submit, fence_in_flight );
    handle_result( result );

    vk::PresentInfoKHR p { };

    p.waitSemaphoreCount = 1;
    p.pWaitSemaphores    = &sema_render_finished;
    p.swapchainCount     = 1;
    p.pSwapchains        = &swap_chain;
    p.pImageIndices      = &index;

    result = queue.presentKHR( &p );

    handle_result( result );
  }

  int match_event( Display*, XEvent* e, XPointer ) {
    if ( e->type == ClientMessage )
      return True;
    else
      return False;
  }

  void fill_vertex_buffer( GPU_Memory< Component >&        mem,
                           const std::vector< Component >& input ) {
    for ( size_t i = 0; i < input.size( ); ++i ) {
      mem[i] = input[i];
    }
  }

  void Engine::loop( ) {
    Window_handle::Event event;
    auto                 last_time = std::chrono::high_resolution_clock::now( );

    while ( true ) {
      event = win.next_event( );
      if ( event.type != Window_handle::Event::Type::Nothing ) {
        if ( event.type == Window_handle::Event::Type::Close_window )
          break;
      }

      auto current = std::chrono::high_resolution_clock::now( );
      auto diff    = std::chrono::duration_cast< std::chrono::milliseconds >(
        current - last_time );
      last_time = current;

      for ( auto& vertex : vertices ) {
        vertex.time += diff.count( ) / 1000.0;
      }

      fill_vertex_buffer( memory, vertices );

      draw_frame( );
    }

    (void)device.waitIdle( );
  }

  Engine::~Engine( ) {
    // instance and/or device manage window and display.
    // freeing them before window and display causes a segmentation fault.
    memory.~GPU_Memory( );
    device.freeCommandBuffers( command_pool, 1, &command_buffer );
    device.destroy( command_pool, nullptr );
    device.destroy( sema_image_available, nullptr );
    device.destroy( sema_render_finished, nullptr );
    device.destroy( fence_in_flight, nullptr );

    free_framebuffers( device, framebuffers );

    device.destroy( graphics_pipeline, nullptr );
    device.destroy( pipeline_cache, nullptr );
    device.destroy( render_pass, nullptr );
    device.destroy( pipeline_layout, nullptr );
    for ( auto& image_view : swap_chain_image_views ) {
      device.destroy( image_view, nullptr );
    }
    device.destroy( swap_chain,
                    nullptr ); // swapchain uses X11 stuff
    instance.destroy( surface, nullptr );
    win.~Window_handle( );
    device.destroy( nullptr );
    instance.destroy( nullptr );
  }

} // namespace ds
