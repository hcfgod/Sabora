#pragma once

// Include chrono early to prevent Windows time.h conflicts when including Time.h
#include <chrono>

#include "Core/Log.h"
#include "Core/Result.h"
#include "Core/AsyncIO.h"
#include "Core/ConfigurationManager.h"
#include "Core/Application.h"
#include "Core/EventManager.h"
#include "Core/Time.h"
#include "Input/KeyCode.h"
#include "Input/Input.h"

// Include EntryPoint.h only if SABORA_USE_ENTRY_POINT is defined
// This allows applications to use the engine's main() while tests can use their own
#ifdef SABORA_USE_ENTRY_POINT
#include "Core/EntryPoint.h"
#endif