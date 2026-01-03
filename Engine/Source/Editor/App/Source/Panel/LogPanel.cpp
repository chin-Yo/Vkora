#include "Panel/LogPanel.hpp"

#include "Logging/ImguiSink.hpp"
#include "Logging/Logger.hpp"

LogPanel::LogPanel()
{
}

void LogPanel::Init()
{
    Panel::Init();
}

void LogPanel::OnUIRender()
{
    std::weak_ptr<spdlog::sinks::sink> sink = Logger::GetLogger()->sinks()[0];
    if (auto shared_base = sink.lock())
    {
        auto custom_sink = std::dynamic_pointer_cast<imgui_sink>(shared_base);

        if (custom_sink)
        {
            custom_sink->Draw("LogPanel");
        }
    }
}
