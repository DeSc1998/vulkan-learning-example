
#include "pch.hpp"

#include "component.hpp"
#include "impl.hpp"
#include "utility.hpp"

namespace ds {

  [[maybe_unused]] static constexpr auto screen_width  = 1200;
  [[maybe_unused]] static constexpr auto screen_height = 800;

  static const std::vector< const char* > validation_layers
    = { "VK_LAYER_KHRONOS_validation" };

  static const std::vector< const char* > device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,
  };

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

    auto extentions = win.get_required_extensions( );
    if constexpr ( debug_mode ) {
      extentions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    extentions.push_back(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );

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
        instance, dev, indicies.graphics_family.value( ) );

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

    info( "picking physical device" );

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

    auto format = std::find_if(
      formats.begin( ), formats.end( ), []( const vk::SurfaceFormatKHR& f ) {
        return f.format == vk::Format::eB8G8R8A8Srgb
               && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
      } );

    return format != formats.end( ) ? *format : *formats.begin( );
  }

  void Engine::create_swap_chain( size_t width, size_t height ) {
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

      return mode != modes.end( ) ? *mode : vk::PresentModeKHR::eFifo;
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

    auto extent = win.get_extent( physical_device, surface );

    if ( width > 0 && height > 0 ) {
      extent.height = height;
      extent.width  = width;
    }

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

    if ( swap_chain ) {
      create_info.oldSwapchain = swap_chain;
    }

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

    auto result
      = device.createSwapchainKHR( &create_info, nullptr, &swap_chain );

    exit_on_fail( "failed to create Swapchain", result );

    if ( create_info.oldSwapchain ) {
      device.destroy( create_info.oldSwapchain, nullptr );
    }

    if constexpr ( debug_mode )
      fmt::print( "INFO: finished creating swap chain\n" );

    (void)device.getSwapchainImagesKHR( swap_chain, &count, nullptr );

    swap_chain_images.resize( count );
    result = device.getSwapchainImagesKHR( swap_chain, &count,
                                           swap_chain_images.data( ) );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to retrive images", result );
    }
    swap_chain_ok = true;

    win.update_extent( physical_device, surface );
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

  void Engine::create_descriptor_layout( ) {
    vk::DescriptorSetLayoutBinding layout_binding { };
    layout_binding.binding         = 0;
    layout_binding.descriptorType  = vk::DescriptorType::eUniformBuffer;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags      = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutCreateInfo create_info { };
    create_info.bindingCount = 1;
    create_info.pBindings    = &layout_binding;

    auto result = device.createDescriptorSetLayout( &create_info, nullptr,
                                                    &descriptor_layout );

    exit_on_fail( "failed to create DescriptorSetLayout", result );
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
    p.setLayoutCount = 1;
    p.pSetLayouts    = &descriptor_layout;

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

    // if constexpr ( debug_mode )
    //   fmt::print( "INFO: framebuffer count: {}\n", framebuffers.size( ) );
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
    // front
    { { 0.5, -0.5, 0.5 }, { 1, 0, 0, 1 }, 0 },
    { { 0, 0.7, 0 }, { 0, 1, 0, 1 }, 0 },
    { { -0.5, -0.5, 0.5 }, { 0, 0, 1, 1 }, 0 },
    // left
    { { -0.5, -0.5, 0.5 }, { 0, 0, 1, 1 }, 0 },
    { { 0, 0.7, 0 }, { 0, 1, 0, 1 }, 0 },
    { { -0.5, -0.5, -0.5 }, { 0, 1, 1, 1 }, 0 },
    // right
    { { 0.5, -0.5, -0.5 }, { 1, 0, 0, 1 }, 0 },
    { { 0, 0.7, 0 }, { 0, 1, 0, 1 }, 0 },
    { { 0.5, -0.5, 0.5 }, { 0, 0, 1, 1 }, 0 },
    // back
    { { -0.5, -0.5, -0.5 }, { 0, 1, 1, 1 }, 0 },
    { { 0, 0.7, 0 }, { 0, 1, 0, 1 }, 0 },
    { { 0.5, -0.5, -0.5 }, { 1, 0, 1, 1 }, 0 },
  };

  Camera cam = { { 0, 1.5, 1.75, 0 }, { 0, -1, -1, 0 } };

  void Engine::set_components( std::vector< Component > data ) {
    user_vertices = data;
  }

  void Engine::create_buffers( ) {
    auto find_mem_type = [&]( uint32_t filter, vk::MemoryPropertyFlags flags ) {
      vk::PhysicalDeviceMemoryProperties mem_props;
      physical_device.getMemoryProperties( &mem_props );
      for ( size_t i = 0; i < mem_props.memoryTypeCount; ++i ) {
        if ( filter & ( 1 << i )
             && ( mem_props.memoryTypes[i].propertyFlags & flags ) == flags ) {
          return i;
        }
      }

      error( "failed to find suitable memory" );
      exit( 1 );
    };

    // uniforms
    using size_type                = vk::DeviceSize;
    const auto uniform_buffer_size = sizeof( Camera );
    uniforms.resize( framebuffers.size( ) );

    for ( auto& uniform : uniforms ) {
      vk::BufferCreateInfo buffer_info { };
      buffer_info.size  = uniform_buffer_size;
      buffer_info.usage = vk::BufferUsageFlagBits::eUniformBuffer
                          | vk::BufferUsageFlagBits::eShaderDeviceAddress;
      buffer_info.sharingMode = vk::SharingMode::eExclusive;
      auto result
        = device.createBuffer( &buffer_info, nullptr, &uniform.buffer );
      exit_on_fail( "failed to create uniform buffer", result );

      vk::MemoryRequirements requirements { };
      device.getBufferMemoryRequirements( uniform.buffer, &requirements );

      vk::MemoryAllocateInfo alloc_info { };
      alloc_info.allocationSize = requirements.size;
      alloc_info.memoryTypeIndex
        = find_mem_type( requirements.memoryTypeBits,
                         vk::MemoryPropertyFlagBits::eHostVisible
                           | vk::MemoryPropertyFlagBits::eHostCoherent );
      result = device.allocateMemory( &alloc_info, nullptr, &uniform.memory );
      exit_on_fail( "failed to create uniform buffer memory", result );

      // TODO: might want to handle the result
      (void)device.bindBufferMemory( uniform.buffer, uniform.memory, 0 );

      result = device.mapMemory(
        uniform.memory, size_type { 0 }, size_type { uniform_buffer_size },
        vk::MemoryMapFlags { 0 }, (void**)&uniform.mapped_memory );
      exit_on_fail( "failed to map uniform buffer memory", result );

      for ( size_t i = 0; i < 3; ++i ) {
        uniform.mapped_memory->direction[i] = cam.direction[i];
        uniform.mapped_memory->position[i]  = cam.position[i];
      }
    }

    // vertex
    if ( user_vertices ) {
      const auto& tmp = user_vertices.value( );
      memory = GPU_Memory< Component > { device, physical_device, tmp.size( ) };

      for ( size_t i = 0; i < tmp.size( ); ++i ) {
        memory[i] = tmp[i];
      }
    } else {
      memory
        = GPU_Memory< Component > { device, physical_device, vertices.size( ) };

      for ( size_t i = 0; i < vertices.size( ); ++i ) {
        memory[i] = vertices[i];
      }
    }
  }

  void Engine::create_descriptor_pool( ) {
    vk::DescriptorPoolSize pool_size { };
    pool_size.descriptorCount = framebuffers.size( );

    vk::DescriptorPoolCreateInfo create_info { };
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes    = &pool_size;
    create_info.maxSets       = framebuffers.size( );

    auto result = device.createDescriptorPool( &create_info, nullptr, &pool );
    exit_on_fail( "failed to create decriptor pool", result );
  }

  void Engine::create_descriptor_sets( ) {
    std::vector< vk::DescriptorSetLayout > layouts { framebuffers.size( ),
                                                     descriptor_layout };
    vk::DescriptorSetAllocateInfo          alloc_info { };
    alloc_info.descriptorPool     = pool;
    alloc_info.descriptorSetCount = framebuffers.size( );
    alloc_info.pSetLayouts        = layouts.data( );

    sets.resize( framebuffers.size( ) );
    auto result = device.allocateDescriptorSets( &alloc_info, sets.data( ) );
    exit_on_fail( "failed to allocate descriptor sets", result );

    for ( size_t i = 0; i < framebuffers.size( ); ++i ) {
      vk::DescriptorBufferInfo info { };
      info.buffer = uniforms[i].buffer;
      info.offset = 0;
      info.range  = sizeof( Camera );

      vk::WriteDescriptorSet write { };
      write.dstSet          = sets[i];
      write.dstBinding      = 0;
      write.dstArrayElement = 0;
      write.descriptorType  = vk::DescriptorType::eUniformBuffer;
      write.descriptorCount = 1;
      write.pBufferInfo     = &info;

      device.updateDescriptorSets( 1, &write, 0, nullptr );
      fmt::print( "INFO: writing descriptor set at index {}\n", i );
    }
  }

  void Engine::record_command_buffer( size_t image_index,
                                      size_t current_frame ) {
    vk::CommandBufferBeginInfo         cb_info { };
    vk::RenderPassBeginInfo            rp_info { };
    vk::DeviceGroupRenderPassBeginInfo grp_info { };

    vk::ClearValue clear_color { std::array< float, 4 > { 0.1, 0.1, 0.1,
                                                          0.2 } };
    vk::Viewport   view { };
    vk::Rect2D     scissor { };
    vk::Rect2D     error_area { };
    auto           extent = win.get_extent( physical_device, surface );

    view.x        = 0.0;
    view.y        = 0.0;
    view.width    = extent.width;
    view.height   = extent.height;
    view.minDepth = 0.0;
    view.maxDepth = 1.0;

    scissor.extent = extent;

    rp_info.renderPass        = render_pass;
    rp_info.framebuffer       = framebuffers[image_index];
    rp_info.renderArea.extent = extent;
    rp_info.clearValueCount   = 1;
    rp_info.pClearValues      = &clear_color;
    rp_info.pNext             = &grp_info;

    error_area.extent.height
      = std::max( extent.height - extent.height / 50, extent.height - 2 );
    error_area.extent.width
      = std::max( extent.width - extent.width / 50, extent.width - 2 );
    grp_info.deviceMask            = 1; // TODO: deviceMask is hard coded
    grp_info.deviceRenderAreaCount = 1;
    grp_info.pDeviceRenderAreas    = &error_area;

    cb_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk::DeviceSize offsets[] = { 0 };

    // TODO: add descriptor
    (void)command_buffer[current_frame].begin( &cb_info );
    command_buffer[current_frame].bindVertexBuffers(
      0, 1, memory.expose_buffer( ), offsets );
    command_buffer[current_frame].beginRenderPass(
      &rp_info, vk::SubpassContents::eInline );
    command_buffer[current_frame].bindPipeline(
      vk::PipelineBindPoint::eGraphics, graphics_pipeline );
    command_buffer[current_frame].setViewport( 0, 1, &view );
    command_buffer[current_frame].setScissor( 0, 1, &scissor );
    command_buffer[current_frame].bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1,
      &sets[current_frame], 0, nullptr );
    if ( user_vertices )
      command_buffer[current_frame].draw( user_vertices.value( ).size( ), 1, 0,
                                          0 );
    else
      command_buffer[current_frame].draw( vertices.size( ), 1, 0, 0 );
    command_buffer[current_frame].endRenderPass( );
    auto result = command_buffer[current_frame].end( );

    if ( result != vk::Result::eSuccess ) {
      error( "an error accured during recording", result );
    }
  }

  void Engine::create_command_buffer( ) {
    vk::CommandBufferAllocateInfo cba { };

    cba.commandPool        = command_pool;
    cba.level              = vk::CommandBufferLevel::ePrimary;
    cba.commandBufferCount = framebuffers.size( );

    command_buffer.resize( framebuffers.size( ) );
    auto result = device.allocateCommandBuffers( &cba, command_buffer.data( ) );

    if ( result != vk::Result::eSuccess ) {
      error( "failed to allocate command buffers", result );
    }
  }

  void Engine::create_sync( ) {
    vk::SemaphoreCreateInfo s { };
    vk::FenceCreateInfo     f { };

    f.flags = vk::FenceCreateFlagBits::eSignaled;

    sema_image_available.resize( framebuffers.size( ) );
    sema_render_finished.resize( framebuffers.size( ) );
    fence_in_flight.resize( framebuffers.size( ) );

    for ( size_t i = 0; i < framebuffers.size( ); ++i ) {
      (void)device.createSemaphore( &s, nullptr, &sema_image_available[i] );
      (void)device.createSemaphore( &s, nullptr, &sema_render_finished[i] );
      (void)device.createFence( &f, nullptr, &fence_in_flight[i] );
    }
  }

  void Engine::init_vulkan( ) {
    create_instance( );
    create_surface( );
    pick_physical_device( );
    create_device( );
    create_swap_chain( );
    create_image_views( );
    create_render_pass( );
    create_descriptor_layout( );
    create_graphics_pipeline( );
    create_framebuffers( );
    create_command_pool( );
    create_command_buffer( );
    create_buffers( );
    create_descriptor_pool( );
    create_descriptor_sets( );
    create_sync( );
  }

  [[nodiscard]] bool recover_device( vk::Device& dev ) {
    // TODO: recover device
    Unused( dev );

    return false;
  }

  void Engine::recreate( ) {
    // TODO: https://nanokatze.gitlab.io/vulkan/handling-window-resize/
    (void)device.waitIdle( );

    if constexpr ( debug_mode )
      info( "freeing image views" );

    for ( size_t i = 0; i < framebuffers.size( ); ++i ) {
      (void)device.waitForFences( 1, &fence_in_flight[i], vk::Bool32( true ),
                                  std::numeric_limits< uint64_t >::max( ) );
      device.destroy( framebuffers[i], nullptr );
    }
    framebuffers.clear( );

    for ( auto& image_view : swap_chain_image_views ) {
      device.destroy( image_view, nullptr );
    }
    swap_chain_image_views.clear( );

    auto new_size = win.from_callback.value_or( Size { 0, 0 } );

    create_swap_chain( new_size.width, new_size.height );
    create_image_views( );
    create_framebuffers( );
  }

  /// @return true if 'result' was vk::Result::eSuccess
  bool Engine::handle_result( vk::Result result ) {
    if ( result == vk::Result::eSuccess )
      return true;

    if ( result == vk::Result::eErrorOutOfDateKHR || win.has_been_resized ) {
      if constexpr ( debug_mode )
        info( "out of date: recreating swap chain and framebuffers" );
      recreate( );
      win.has_been_resized = false;
      if constexpr ( debug_mode )
        info( "finished recreating" );
    } else if ( result == vk::Result::eErrorDeviceLost ) {
      if constexpr ( debug_mode )
        info( "device has been lost" );

      if ( !recover_device( device ) ) {
        error( "failed to recover device", vk::Result::eErrorDeviceLost );
        this->~Engine( );
        exit( 1 );
      }
    } else if ( result == vk::Result::eTimeout ) {
      if constexpr ( debug_mode )
        info( "an action has been timed out" );
    } else {
      error( "an error accured", result );
    }
    return false;
  }

  /// @return true if a new frame been submitted
  bool Engine::draw_frame( size_t current_frame ) {
    auto result = device.waitForFences(
      1, &fence_in_flight[current_frame], vk::Bool32( true ),
      std::numeric_limits< uint64_t >::max( ) );

    if ( !handle_result( result ) ) {
      info( "after waiting for fence" );
      return false;
    }

    result = device.resetFences( 1, &fence_in_flight[current_frame] );
    if ( !handle_result( result ) )
      return false;

    uint32_t index { };
    result = device.acquireNextImageKHR(
      swap_chain, std::numeric_limits< uint64_t >::max( ),
      sema_image_available[current_frame], nullptr, &index );

    if ( !handle_result( result ) ) {
      info( "after acquiring next image" );
      return false;
    }

    result = command_buffer[current_frame].reset(
      vk::CommandBufferResetFlags { 0 } );
    if ( !handle_result( result ) )
      return false;

    record_command_buffer( index, current_frame );

    vk::SubmitInfo         submit { };
    vk::PipelineStageFlags flags[]
      = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &sema_image_available[current_frame];
    submit.pWaitDstStageMask    = flags;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &command_buffer[current_frame];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &sema_render_finished[current_frame];

    result = queue.submit( 1, &submit, fence_in_flight[current_frame] );
    if ( !handle_result( result ) )
      return false;

    vk::PresentInfoKHR p { };

    p.waitSemaphoreCount = 1;
    p.pWaitSemaphores    = &sema_render_finished[current_frame];
    p.swapchainCount     = 1;
    p.pSwapchains        = &swap_chain;
    p.pImageIndices      = &index;

    result = queue.presentKHR( &p );

    return handle_result( result );
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
    size_t               current_frame = 0;

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

      for ( auto& vertex : user_vertices ? user_vertices.value( ) : vertices ) {
        vertex.time += diff.count( ) / 1000.0;
      }

      if ( user_vertices )
        fill_vertex_buffer( memory, user_vertices.value( ) );
      else
        fill_vertex_buffer( memory, vertices );

      auto memory = *uniforms[current_frame].mapped_memory;
      for ( size_t i = 0; i < 3; ++i ) {
        memory.direction[i] = cam.direction[i];
        memory.position[i]  = cam.position[i];
      }

      if ( draw_frame( current_frame ) )
        current_frame = ( current_frame + 1 ) % framebuffers.size( );
    }

    (void)device.waitIdle( );
  }

  Engine::~Engine( ) {
    if ( !instance )
      return;

    (void)device.waitForFences( fence_in_flight.size( ),
                                fence_in_flight.data( ), vk::Bool32( true ),
                                std::numeric_limits< uint64_t >::max( ) );
    (void)device.waitIdle( );

    memory.~GPU_Memory( );

    for ( auto& uniform : uniforms ) {
      device.unmapMemory( uniform.memory );
      device.freeMemory( uniform.memory, nullptr );
      device.destroy( uniform.buffer, nullptr );
    }

    device.destroy( pool, nullptr );

    device.freeCommandBuffers( command_pool, command_buffer.size( ),
                               command_buffer.data( ) );
    device.destroy( command_pool, nullptr );
    for ( size_t i = 0; i < framebuffers.size( ); ++i ) {
      device.destroy( sema_image_available[i], nullptr );
      device.destroy( sema_render_finished[i], nullptr );
      device.destroy( fence_in_flight[i], nullptr );
    }

    free_framebuffers( device, framebuffers );

    device.destroy( graphics_pipeline, nullptr );
    device.destroy( pipeline_cache, nullptr );
    device.destroy( render_pass, nullptr );
    device.destroy( pipeline_layout, nullptr );
    device.destroy( descriptor_layout, nullptr );
    for ( auto& image_view : swap_chain_image_views ) {
      device.destroy( image_view, nullptr );
    }
    device.destroy( swap_chain,
                    nullptr ); // swapchain uses X11 stuff
    instance.destroy( surface, nullptr );
    device.destroy( nullptr );
    instance.destroy( nullptr );

    info( "~Engine - completed destruction" );
  }

} // namespace ds
