//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <maa_core/maa_core_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) maa_core_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "MaaCorePlugin");
  maa_core_plugin_register_with_registrar(maa_core_registrar);
}
