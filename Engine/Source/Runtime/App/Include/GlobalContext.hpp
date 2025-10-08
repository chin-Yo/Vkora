#pragma once

#include <memory>
#include <string>
#include "WindowSystem.hpp"

class RenderSystem;
class WindowSystem;

struct EngineInitParams;

/// Manage the lifetime and creation/destruction order of all global system
class RuntimeGlobalContext
{
public:
    // create all global systems and initialize these systems
    void StartSystems(const std::string& config_file_path);
    // destroy all global systems
    void ShutdownSystems();

public:
    std::shared_ptr<WindowSystem> windowSystem;
    std::shared_ptr<RenderSystem> renderSystem;
};

extern RuntimeGlobalContext GRuntimeGlobalContext;
