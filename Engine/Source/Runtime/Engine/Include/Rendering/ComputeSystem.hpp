#pragma once
#include <vector>

#include "ComputePipeline/ComputePass.hpp"


class ComputeSystem
{
public:
    ComputeSystem() = default;
    ~ComputeSystem() = default;

    bool PushPass(std::string Name, ComputePassBase* ComputePass);

    void AllPoll();

private:
    std::unordered_map<std::string, ComputePassBase*> ComputePasss;
};
