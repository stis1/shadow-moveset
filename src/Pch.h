#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#undef CreateService
#undef max
#undef min

#include <detours.h>
#include <d3d11.h>

#include <cstdint>
#include <miller-sdk.h>
#include <ini.h>
#include <utilities/Helpers.h>
#include <iostream>
#include <optional>
///////////////////////////////////////
#include <utilities/stisCommon.h>
#include <rfl_idiotism.h>
#include <Patches.h>
#include <Variables.h>
///////////////////////////////////////
