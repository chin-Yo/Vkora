#include "Rendering/ComputeSystem.hpp"


bool ComputeSystem::PushPass(std::string Name, ComputePassBase* ComputePass)
{
    if (!ComputePass) return false;
    auto [it, inserted] = ComputePasss.try_emplace(std::move(Name), ComputePass);
    return inserted;
}

void ComputeSystem::AllPoll()
{
    for (auto& pair : ComputePasss)
    {
        pair.second->Poll();
    }
}
