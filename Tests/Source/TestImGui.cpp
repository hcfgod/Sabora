/**
 * @file TestImGui.cpp
 * @brief Smoke test to validate Dear ImGui (docking) integration.
 */

#include "doctest.h"
#include <imgui.h>
#include <string>

TEST_SUITE("ImGui")
{
    TEST_CASE("Create and destroy context")
    {
        ImGui::CreateContext();
        const char* version = ImGui::GetVersion();
        CHECK(version != nullptr);
        CHECK(std::string(version).find(IMGUI_VERSION) != std::string::npos);

        // Default flags sanity
        auto& io = ImGui::GetIO();
        CHECK((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) == 0); // docking opt-in by user

        ImGui::DestroyContext();
    }
}