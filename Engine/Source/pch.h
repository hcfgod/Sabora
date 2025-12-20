/**
 * @file pch.h
 * @brief Precompiled header for Sabora Engine.
 * 
 * This header includes commonly used standard library headers and engine core headers
 * to speed up compilation times. Include this file as the first include in source files.
 */

#pragma once

// Standard library headers
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <vector>

// Engine core headers
#include "Core/Result.h"
#include "Core/Log.h"