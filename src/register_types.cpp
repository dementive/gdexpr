#include "register_types.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "GDExpr.hpp"
#include "GDExprExample.hpp"

using namespace godot;
using namespace gdexpr;

void initialize_gdexpr_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_ABSTRACT_CLASS(GDExprBase)
	GDREGISTER_ABSTRACT_CLASS(BaseGDExprScript)
	GDREGISTER_CLASS(GDExpr)
#ifdef GDEXPR_COMPILER_DEBUG
	GDREGISTER_CLASS(GDExprExampleScript)
#endif
	Engine::get_singleton()->register_singleton("GDExpr", memnew(GDExpr));
}

void uninitialize_gdexpr_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT gdextension_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_gdexpr_module);
	init_obj.register_terminator(uninitialize_gdexpr_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
