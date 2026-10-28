// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "platform/types.hpp"
#include "settings.hpp"
#include <memory>
#include <string>
namespace arcade::native {
// Direct Vulkan game renderer. OpenGL is used only by the launcher and pause menu.
class VulkanProcessor {
public:
 VulkanProcessor();~VulkanProcessor();
 bool open(std::string& error,const std::string& assets="");
 bool process(const Frame&,const Settings&,Frame& output,std::string& error);
 bool open_display(const Settings&,std::string& error);
 void overlay(const std::string&,const Settings&);
 bool present(std::string& error);
 void close_display();
 bool display_open()const;
 void close();
private:struct State;std::unique_ptr<State> state_;
};
}
