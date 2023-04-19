#define NOMINMAX

#include <filesystem>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppModule.hpp>

namespace RC
{
    CppModule::CppModule(UE4SSProgram& program, std::wstring&& mod_name, std::wstring&& mod_path) : Mod(program, std::move(mod_name), std::move(mod_path))
    {
        m_dlls_path = m_mod_path + L"\\dlls";

        if (!std::filesystem::exists(m_dlls_path))
        {
            Output::send<LogLevel::Warning>(STR("Could not find the dlls folder for mod %s"), m_mod_name);
            set_installable(false);
            return;
        }

        auto dll_path = m_dlls_path + L"\\main.dll";
        m_main_dll_module = LoadLibraryW(dll_path.c_str());

        if (!m_main_dll_module)
        {
            Output::send<LogLevel::Warning>(STR("Failed to load dll <%s> for mod %s, error code: %d"), dll_path, m_mod_name, GetLastError());
            set_installable(false);
            return;
        }

        m_start_mod_func = reinterpret_cast<start_type>(GetProcAddress(m_main_dll_module, "start_mod"));
        m_uninstall_mod_func = reinterpret_cast<uninstall_type>(GetProcAddress(m_main_dll_module, "uninstall_mod"));

        if (!m_start_mod_func || !m_uninstall_mod_func)
        {
            Output::send<LogLevel::Warning>(STR("Failed to find exported mod lifecycle functions for mod %s"), m_mod_name);

            FreeLibrary(m_main_dll_module);
            m_main_dll_module = NULL;

            set_installable(false);
            return;
        }
    }

    auto CppModule::start_mod() -> void
    {
        m_mod = m_start_mod_func();
        m_is_started = m_mod != nullptr;
    }

    auto CppModule::uninstall() -> void
    {
        if (m_mod && m_uninstall_mod_func) { m_uninstall_mod_func(m_mod); }
    }

    auto CppModule::update() -> void
    {
        if (m_mod) { m_mod->update(); }
    }

    CppModule::~CppModule()
    {
        if (m_main_dll_module) { FreeLibrary(m_main_dll_module); }
    }
}