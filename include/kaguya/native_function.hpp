#pragma once

#include <string>
#include <vector>

#include "kaguya/config.hpp"
#include "kaguya/utility.hpp"
#include "kaguya/type.hpp"
#include "kaguya/lua_ref.hpp"


#if KAGUYA_USE_CPP11
#include "native_function_cxx11.hpp"
#else
#include "native_function_cxx03.hpp"
#endif
namespace kaguya
{
	class VariadicArgType
	{
	public:
		VariadicArgType(lua_State* state, int startIndex) :state_(state), startIndex_(startIndex), endIndex_(lua_gettop(state) + 1)
		{
		}

		template<typename T>
		operator std::vector<T>()const
		{
			if (startIndex_ >= endIndex_)
			{
				return std::vector<T>();
			}
			std::vector<T> result;
			result.reserve(endIndex_ - startIndex_);
			for (int index = startIndex_; index < endIndex_; ++index)
			{
				result.push_back(lua_type_traits<T>::get(state_, index));
			}
			return result;
		}
	private:
		int startIndex_;
		int endIndex_;
		lua_State* state_;
	};
	template<> struct lua_type_traits<VariadicArgType>
	{
		typedef VariadicArgType get_type;

		static bool strictCheckType(lua_State* l, int index)
		{
			return true;
		}
		static bool checkType(lua_State* l, int index)
		{
			return true;
		}
		static get_type get(lua_State* l, int index)
		{
			return VariadicArgType(l, index);
		}
	};

	namespace nativefunction
	{
		struct BaseInvoker
		{
			virtual int argsCount()const = 0;
			virtual bool checktype(lua_State *state, bool strictcheck) = 0;
			virtual int invoke(lua_State *state) = 0;
			virtual std::string argumentTypeNames() = 0;
			virtual ~BaseInvoker() {}
		};

		struct FunctorType :standard::shared_ptr<BaseInvoker>
		{
			typedef standard::shared_ptr<BaseInvoker> base_ptr_;
			FunctorType() {}
			FunctorType(const standard::shared_ptr<BaseInvoker>& ptr) :base_ptr_(ptr) {}

			template<typename T>FunctorType(T f,bool typecheck=true) : standard::shared_ptr<BaseInvoker>(create(f, typecheck))
			{
			}
		private:
			template<typename F>
			struct FunInvoker :BaseInvoker {
				typedef F func_type;
				func_type func_;
				bool typecheck_;
				FunInvoker(func_type fun, bool typecheck) :func_(fun), typecheck_(typecheck){}
				virtual int argsCount()const {
					return argCount(func_);
				}
				virtual bool checktype(lua_State *state, bool strictcheck) {
					if (!typecheck_) { return true; }
					if (strictcheck)
					{
						return strictCheckArgTypes(state, func_);
					}
					else
					{
						return checkArgTypes(state, func_);
					}
				}
				virtual int invoke(lua_State *state)
				{
					return call(state, func_);
				}
				virtual std::string argumentTypeNames() {
					return argTypesName(func_);
				}
			};
			template<typename F>
			static base_ptr_ create(F fun,bool typecheck)
			{
				typedef FunInvoker<F> InvokerType;
				return base_ptr_(new InvokerType(fun, typecheck));
			}
		};

		inline FunctorType* pick_match_function(lua_State *l)
		{
			int argcount = lua_gettop(l);
			int overloadnum = int(lua_tonumber(l, lua_upvalueindex(1)));

			if (overloadnum == 1)
			{
				FunctorType* fun = static_cast<FunctorType*>(lua_touserdata(l, lua_upvalueindex(2)));

				if (!(*fun)->checktype(l, false))
				{
					return 0;
				}
				return fun;
			}
			FunctorType* weak_match = 0;
			FunctorType* argcount_unmatch = 0;
			for (int i = 0; i < overloadnum; ++i)
			{
				FunctorType* fun = static_cast<FunctorType*>(lua_touserdata(l, lua_upvalueindex(i + 2)));
				if (!fun || !(*fun))
				{
					continue;
				}
				int fnarg = (*fun)->argsCount();
				bool match_argcount = fnarg == argcount;
				if (match_argcount && (*fun)->checktype(l, true))
				{
					return fun;
				}
				else if (weak_match == 0 && (match_argcount || !argcount_unmatch) && (*fun)->checktype(l, false))
				{
					if (match_argcount)
					{
						weak_match = fun;
					}
					else
					{
						argcount_unmatch = fun;
					}
				}
			}
			return weak_match ? weak_match : argcount_unmatch;
		}
		inline std::string build_arg_error_message(lua_State *l)
		{
			std::string message = "argument not matching:" + util::argmentTypes(l) + "\t candidated\n";

			int overloadnum = int(lua_tonumber(l, lua_upvalueindex(1)));
			for (int i = 0; i < overloadnum; ++i)
			{
				FunctorType* fun = static_cast<FunctorType*>(lua_touserdata(l, lua_upvalueindex(i + 2)));
				if (!fun || !(*fun))
				{
					continue;
				}
				message += std::string("\t\t") + (*fun)->argumentTypeNames() + "\n";
			}
			return message;
		}
		inline int functor_dispatcher(lua_State *l)
		{
			FunctorType* fun = pick_match_function(l);
			if (fun && (*fun))
			{
				try {
					return (*fun)->invoke(l);
				}
				catch (std::exception & e) {
					util::traceBack(l, e.what());
				}
				catch (...) {
					util::traceBack(l, "Unknown exception");
				}
			}
			else
			{
				util::traceBack(l, build_arg_error_message(l).c_str());
			}
			return lua_error(l);
		}

		inline int functor_destructor(lua_State *state)
		{
			FunctorType* f = class_userdata::test_userdata<FunctorType>(state, 1);
			if (f)
			{
				f->~FunctorType();
			}
			return 0;
		}
		inline void reg_functor_destructor(lua_State* state)
		{
			if (class_userdata::newmetatable<FunctorType>(state))
			{
				lua_pushcclosure(state, &functor_destructor, 0);
				lua_setfield(state, -2, "__gc");
				lua_setfield(state, -1, "__index");
			}
		}
	}

	typedef nativefunction::FunctorType FunctorType;


	template<typename T>
	inline FunctorType lua_function(T f)
	{
		return FunctorType(standard::forward<T>(f));
	}

	template<typename T>
	inline FunctorType function(T f)
	{
		return FunctorType(standard::forward<T>(f));
	}

	template<>	struct lua_type_traits<FunctorType> {
		typedef FunctorType get_type;
		typedef FunctorType push_type;

		static bool strictCheckType(lua_State* l, int index)
		{
			return 0 != class_userdata::test_userdata<FunctorType>(l, index);
		}
		static bool checkType(lua_State* l, int index)
		{
			return 0 != class_userdata::test_userdata<FunctorType>(l, index);
		}
		static FunctorType get(lua_State* l, int index)
		{
			FunctorType* ptr = class_userdata::test_userdata<FunctorType>(l, index);
			if (ptr)
			{
				return *ptr;
			}
			return FunctorType();
		}
		static int push(lua_State* l, const FunctorType& f)
		{
			lua_pushnumber(l, 1);//no overload
			void *storage = lua_newuserdata(l, sizeof(FunctorType));
			new(storage) FunctorType(f);
			class_userdata::setmetatable<FunctorType>(l);
			lua_pushcclosure(l, &nativefunction::functor_dispatcher, 2);
			return 1;
		}
#if KAGUYA_USE_RVALUE_REFERENCE
		static int push(lua_State* l, FunctorType&& f)
		{
			lua_pushnumber(l, 1);//no overload
			void *storage = lua_newuserdata(l, sizeof(FunctorType));
			new(storage) FunctorType(standard::forward<FunctorType>(f));
			class_userdata::setmetatable<FunctorType>(l);
			lua_pushcclosure(l, &nativefunction::functor_dispatcher, 2);
			return 1;
		}
#endif
	};

	template<>	struct lua_type_traits<const FunctorType&> :lua_type_traits<FunctorType> {};


	template<typename T> struct lua_type_traits < T
		, typename traits::enable_if<traits::is_function<typename traits::remove_pointer<T>::type>::value>::type > :lua_type_traits<FunctorType> {};

};
