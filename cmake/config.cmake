set(debug_comp_defs "_DEBUG;ASST_DEBUG")
add_compile_definitions("$<$<CONFIG:Debug>:${debug_comp_defs}>")
