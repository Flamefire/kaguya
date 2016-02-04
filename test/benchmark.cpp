#include "kaguya/kaguya.hpp"

#include "benchmark_function.hpp"



typedef void(*benchmark_function_t)(kaguya::State&);
typedef std::vector<std::pair<std::string, benchmark_function_t> > benchmark_function_map_t;
void empty(kaguya::State&)
{

}

void execute_benchmark(const benchmark_function_map_t& testmap)
{
	for (benchmark_function_map_t::const_iterator it = testmap.begin(); it != testmap.end(); ++it)
	{
		const std::string& test_name = it->first;


		double total_time = 0;
		static const int N = 4;
		for (int i = 0; i < N; ++i)
		{
			kaguya::State state;
			double start = state["os"]["clock"]();

			it->second(state);

			double end = state["os"]["clock"]();
			total_time += end - start;
		}

		std::cout << test_name << " average time:" << total_time/N << std::endl;
	}
}


int main()
{
	benchmark_function_map_t functionmap;
#define ADD_BENCHMARK(function) functionmap.push_back(std::make_pair(#function,&function));
	ADD_BENCHMARK(empty);
	ADD_BENCHMARK(kaguya_api_benchmark______::simple_get_set);
	ADD_BENCHMARK(kaguya_api_benchmark______::simple_get_set_without_typecheck);
	ADD_BENCHMARK(kaguya_api_benchmark______::simple_get_set_include_property);
	ADD_BENCHMARK(kaguya_api_benchmark______::property_access);
	ADD_BENCHMARK(kaguya_api_benchmark______::object_pointer_register_get_set);
	ADD_BENCHMARK(kaguya_api_benchmark______::call_native_function);
	ADD_BENCHMARK(original_api_no_type_check::call_native_function);
	ADD_BENCHMARK(kaguya_api_benchmark______::call_lua_function);
	ADD_BENCHMARK(original_api_no_type_check::call_lua_function);
	ADD_BENCHMARK(kaguya_api_benchmark______::call_lua_function_operator_functional);
	ADD_BENCHMARK(kaguya_api_benchmark______::lua_table_access);
	ADD_BENCHMARK(original_api_no_type_check::lua_table_access);
	ADD_BENCHMARK(kaguya_api_benchmark______::lua_table_bracket_operator_access);
	
	

	execute_benchmark(functionmap);

}
