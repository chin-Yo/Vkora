#pragma once

#include "imgui.h"
#include <vector>
#include <string>
#include <algorithm>

#include "Misc/Profiler.hpp"

class ProfileDrawer
{
public:
    static void RenderProfileNode(const ProfileResult& node, double totalFrameTime);

    static void ShowProfilerWindow(const std::vector<ProfileResult>& rootNodes, bool* p_open = nullptr);
};
