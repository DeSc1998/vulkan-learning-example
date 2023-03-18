#pragma once

// stl
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#ifdef WAYLAND
#include <wayland-client.h>

#define VK_USE_PLATFORM_WAYLAND_KHR
#define VULKAN_HPP_DISABLE_ENHANCED_MODE

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_wayland.h>
#else
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

#define VK_USE_PLATFORM_XLIB_KHR
#define VULKAN_HPP_DISABLE_ENHANCED_MODE

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_xlib.h>
#endif

// external libs
#include <fmt/format.h>

#ifdef NDEBUG
static constexpr auto debug_mode = false;
#else
static constexpr auto debug_mode = true;
#endif
