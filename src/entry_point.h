// WARNING: Do not include this file more than once in your game project.

#pragma once

#include "application.h"
#include <spdlog/spdlog.h>

// Define this in your application
namespace afre { Application CreateApplication(); }

// This is not a good way of doing it...
int main()
{
    spdlog::set_pattern("%^[%l] %v%$");

    afre::CreateApplication();

    return 0;
}