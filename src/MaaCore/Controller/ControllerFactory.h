#pragma once

#include "AdbController.h"
#include "ControllerAPI.h"
#include "CustomController.h"
#include "MaatouchController.h"
#include "MinitouchController.h"
#include "PlayToolsController.h"

namespace asst
{
    class ControllerFactory
    {
    public:
        ControllerFactory(const AsstCallback& callback, Assistant* inst, AsstCustomController* custom)
            : m_callback(callback), m_inst(inst), m_custom_controller(custom)
        {}
        ~ControllerFactory() = default;

        std::shared_ptr<ControllerAPI> create_controller(ControllerType type, const std::string& adb_path,
                                                         const std::string& address, const std::string& config,
                                                         PlatformType platform_type)
        {
            std::shared_ptr<ControllerAPI> controller;
            try {
                switch (type) {
                case ControllerType::Adb:
                    controller = std::make_shared<AdbController>(m_callback, m_inst, platform_type);
                    break;
                case ControllerType::Minitouch:
                    controller = std::make_shared<MinitouchController>(m_callback, m_inst, platform_type);
                    break;
                case ControllerType::Maatouch:
                    controller = std::make_shared<MaatouchController>(m_callback, m_inst, platform_type);
                    break;
                case ControllerType::MacPlayTools:
                    controller = std::make_shared<PlayToolsController>(m_callback, m_inst, platform_type);
                    break;
                case ControllerType::Custom:
                    controller = std::make_shared<CustomController>(m_callback, m_inst, m_custom_controller);
                    break;
                default:
                    return nullptr;
                }
            }
            catch (const std::exception& e) {
                Log.error("Cannot create controller: {}", e.what());
                return nullptr;
            }
            if (controller->connect(adb_path, address, config)) {
                return controller;
            }
            return nullptr;
        }

    private:
        AsstCallback m_callback;
        Assistant* m_inst;
        std::shared_ptr<AsstCustomController> m_custom_controller;
    };
}
