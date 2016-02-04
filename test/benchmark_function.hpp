#pragma once
namespace kaguya_api_benchmark______
{

	void simple_get_set(kaguya::State& state);
	void simple_get_set_without_typecheck(kaguya::State& state);
	void simple_get_set_include_property(kaguya::State& state);
	
	void object_pointer_register_get_set(kaguya::State& state);

	void call_native_function(kaguya::State& state);

	void call_lua_function(kaguya::State& state);
	void call_lua_function_operator_functional(kaguya::State& state);
	void lua_table_access(kaguya::State& state);
	void lua_table_bracket_operator_access(kaguya::State& state);

	void property_access(kaguya::State& state);
}

namespace original_api_no_type_check
{
	void call_native_function(kaguya::State& state);
	void call_lua_function(kaguya::State& state);
	void lua_table_access(kaguya::State& state);
}