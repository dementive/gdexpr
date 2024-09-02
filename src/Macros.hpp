#ifndef GCMacros_H
#define GCMacros_H

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/classes/window.hpp"

namespace gdexpr {

#define ROOT get_tree()->get_root()->get_node<Galaxy>("Galaxy")

#define GDSINGLETON(m_class)                                                                                                                                                  \
	inline static m_class *singleton = nullptr;                                                                                                                               \
	static m_class *get_singleton();                                                                                                                                          \
                                                                                                                                                                              \
public:                                                                                                                                                                       \
	m_class() {                                                                                                                                                               \
		ERR_FAIL_COND(singleton != nullptr);                                                                                                                                  \
		singleton = this;                                                                                                                                                     \
	}                                                                                                                                                                         \
                                                                                                                                                                              \
	~m_class() {                                                                                                                                                              \
		ERR_FAIL_COND(singleton != this);                                                                                                                                     \
		singleton = nullptr;                                                                                                                                                  \
	}                                                                                                                                                                         \
                                                                                                                                                                              \
private:

#define GDREGISTER_RUNTIME_SINGLETON(m_class)                                                                                                                                 \
	GDREGISTER_RUNTIME_CLASS(m_class)                                                                                                                                         \
	Engine::get_singleton()->register_singleton(#m_class, memnew(m_class));

#define GDREGISTER_SINGLETON(m_class)                                                                                                                                         \
	GDREGISTER_CLASS(m_class)                                                                                                                                                 \
	Engine::get_singleton()->register_singleton(#m_class, memnew(m_class));

#define GET_SINGLETON(m_class, m_variable_name) Ref<m_class> m_variable_name = Ref<m_class>(Engine::get_singleton()->get_singleton(#m_class));

// Measure execution time of m_code with a label m_thing_to_time in microseconds and print the result.
#define TIME_MICRO(m_thing_to_time, m_code)                                                                                                                                   \
	uint64_t X_##m_thing_to_time##_start = Time::get_singleton()->get_ticks_usec();                                                                                           \
	m_code;                                                                                                                                                                   \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to execute ", #m_thing_to_time, ": ", (X_##m_thing_to_time##_end - X_##m_thing_to_time##_start), " microseconds");

// Measure execution time of m_code with a label m_thing_to_time in seconds and print the result.
#define TIME_SECONDS(m_thing_to_time, m_code)                                                                                                                                 \
	uint64_t X_##m_thing_to_time##_start = Time::get_singleton()->get_ticks_usec();                                                                                           \
	m_code;                                                                                                                                                                   \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to run ", #m_thing_to_time, ": ", ((X_##m_thing_to_time##_end - X_##m_thing_to_time##_start) / 1000000.0), " seconds");

#define TIME_START(m_thing_to_time) uint64_t X_##m_thing_to_time##_start = Time::get_singleton()->get_ticks_usec();

#define TIME_MICRO_END(m_thing_to_time)                                                                                                                                       \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to run ", #m_thing_to_time, ": ", (X_##m_thing_to_time##_end - X_##m_thing_to_time##_start), " microseconds");

#define TIME_SECONDS_END(m_thing_to_time)                                                                                                                                     \
	uint64_t X_##m_thing_to_time##_end = Time::get_singleton()->get_ticks_usec();                                                                                             \
	UtilityFunctions::print("Total time taken to run ", #m_thing_to_time, ": ", ((X_##m_thing_to_time##_end - X_##m_thing_to_time##_start) / 1000000.0), " seconds");

#define ADD_PROP(m_type, m_name, m_variant_type, m_hint_type)                                                                                                                 \
	ClassDB::bind_method(D_METHOD("get_" #m_name), &m_type::get_##m_name);                                                                                                    \
	ClassDB::bind_method(D_METHOD("set_" #m_name, "new_" #m_name), &m_type::set_##m_name);                                                                                    \
	ADD_PROPERTY(PropertyInfo(Variant::m_variant_type, #m_name, m_hint_type), "set_" #m_name, "get_" #m_name);

} //namespace gdexpr

#endif