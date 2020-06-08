#pragma once

#include <list>
#include <type_traits>
#include <memory>
#include <map>

template<class R, class... A>
struct MsgHandler
{
	virtual R operator()(A ...) = 0;
};

template<class R, class ...A>
struct GlobalMsgHandler :MsgHandler<R, A...>
{
	R(*func)(A...) = nullptr;

	GlobalMsgHandler(R(*func)(A...)) :
		func(func)
	{}

	virtual R operator()(A ... args) override
	{
		return func ? func(args...) : R();
	}
};

template<class T, class R, class ...A>
struct ObjectiveMsgHandler :MsgHandler<R, A...>
{
	T& obj;
	R(T::* func)(A...) = nullptr;

	ObjectiveMsgHandler(T& obj, R(T::* func)(A ...)) :
		obj(obj), func(func)
	{}

	virtual R operator()(A ... args) override
	{
		return func ? (obj.*func)(args...) : R();
	}
};

template<class T, class R, class ...A>
auto AutoBind(T& obj, R(T::* func)(A ...))
{
	return std::make_unique<ObjectiveMsgHandler<T, R, A...>>(obj, func);
}

template<class R, class ...A>
auto AutoBind(R(*func)(A ...))
{
	return std::make_unique<GlobalMsgHandler<R, A...>>(func);
}

template<class R, class ...A>
struct caller
{
	virtual R operator()(A ...)
	{
		return R();
	}
};

template<class R, class ...A>
struct cell_caller :caller<R, A...>,
	std::list<std::unique_ptr<MsgHandler<R, A...>>>
{
	using base = std::list<std::unique_ptr<MsgHandler<R, A...>>>;
	virtual R operator()(A ...args) override
	{
		if (base::size() == 0) return R{};
		auto iter = base::begin();
		R res = (**iter)(args...);
		for (++iter; iter != base::end(); ++iter)
		{
			(**iter)(args...);
		}
		return res;
	}
};

template<class ...A>
struct cell_caller<void, A...> :caller<void, A...>,
	std::list<std::unique_ptr<MsgHandler<void, A...>>>
{
	virtual void operator()(A ...args) override
	{
		for (auto& fx : *this)
		{
			(*fx)(args...);
		}
	}
};

template<class M, class R, class ...A>
class dispatcher;

template<class M, class R, class ...A>
class dispatcher<M, R(A...)>
{
public:
	using msg_t = M;

	template<class T>
	void register_handler(msg_t msg, T& obj, R(T::* func)(A ...))
	{
		callers[msg].emplace_back(AutoBind(obj, func));
	}

	void register_handler(msg_t msg, R(*func)(A ...))
	{
		callers[msg].emplace_back(AutoBind(func));
	}

	template<class T>
	void unregister_handler(msg_t msg, T& obj, R(T::* func)(A ...))
	{
		unregister(msg, obj, func);
	}

	void unregister_handler(msg_t msg, R(*func)(A ...))
	{
		unregister(msg, func);
	}

	caller<R, A...>& operator[](msg_t msg)
	{
		static caller<R, A...> dummy;
		auto iter = callers.find(msg);
		return iter == callers.end() ?
			dummy :
			iter->second;
	}

protected:
	std::map<msg_t, cell_caller<R, A...>> callers;

	static bool equals(MsgHandler<R, A...>* handler, R(*func)(A ...))
	{
		auto p = dynamic_cast<GlobalMsgHandler<R, A...>*>(handler);
		return p && p->func == func;
	}

	template<class T>
	static bool equals(MsgHandler<R, A...>* handler, T& obj, R(T::* func)(A ...))
	{
		auto p = dynamic_cast<ObjectiveMsgHandler<T, R, A...>*>(handler);
		return p && p->func == func && &p->obj == &obj;
	}

	template<class...T>
	void unregister(msg_t msg, T&& ... args)
	{
		auto iterCaller = callers.find(msg);
		if (iterCaller == callers.end()) return;
		for (auto iterFx = iterCaller->second.begin(); iterFx != iterCaller->second.end(); )
		{
			if (equals(iterFx->get(), std::forward<T>(args)...))
			{
				iterFx = iterCaller->second.erase(iterFx);
			}
			else
			{
				++iterFx;
			}
		}
		if (iterCaller->second.size() == 0)
		{
			callers.erase(iterCaller);
		}
	}
};
