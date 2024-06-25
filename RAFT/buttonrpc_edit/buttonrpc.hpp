/**
*
*	buttonrpc library
*	Copyright 2018-04-28 Button
*
*/

#pragma once
#include <string>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <zmq.hpp>
#include "Serializer.hpp"


template<typename T>
struct type_xx{	typedef T type; };

template<>
struct type_xx<void>{ typedef int8_t type; };

class buttonrpc
{
public:
	enum rpc_role{
		RPC_CLIENT,
		RPC_SERVER
	};
	enum rpc_err_code {
		RPC_ERR_SUCCESS = 0,
		RPC_ERR_FUNCTIION_NOT_BIND,
		RPC_ERR_RECV_TIMEOUT
	};
	// return value
	template<typename T>
	class value_t {
	public:
		typedef typename type_xx<T>::type type;
		typedef std::string msg_type;
		typedef uint16_t code_type;

		value_t() { code_ = 0; msg_.clear(); }
		bool valid() { return (code_ == 0 ? true : false); }
		int error_code() { return code_; }
		std::string error_msg() { return msg_; }
		type val() { return val_; }

		void set_val(const type& val) { val_ = val; }
		void set_code(code_type code) { code_ = code; }
		void set_msg(msg_type msg) { msg_ = msg; }

		friend Serializer& operator >> (Serializer& in, value_t<T>& d) {
			in >> d.code_ >> d.msg_;
			if (d.code_ == 0) {
				in >> d.val_;
			}
			return in;
		}
		friend Serializer& operator << (Serializer& out, value_t<T> d) {
			out << d.code_ << d.msg_ << d.val_;
			return out;
		}
	private:
		code_type code_;
		msg_type msg_;
		type val_;
	};

	buttonrpc();
	~buttonrpc();

	// network
	void as_client(std::string ip, int port);
	void as_server(int port);
	void send(zmq::message_t& data);
	void recv(zmq::message_t& data);
	void set_timeout(uint32_t ms);
	void run();

public:
	// server
	template<typename F>
	void bind(std::string name, F func);

	template<typename F, typename S>
	void bind(std::string name, F func, S* s);

	// client
	template<typename R>
	value_t<R> call(std::string name);

	template<typename R, typename P1>
	value_t<R> call(std::string name, P1);

	template<typename R, typename P1, typename P2>
	value_t<R> call(std::string name, P1, P2);

	template<typename R, typename P1, typename P2, typename P3>
	value_t<R> call(std::string name, P1, P2, P3);

	template<typename R, typename P1, typename P2, typename P3, typename P4>
	value_t<R> call(std::string name, P1, P2, P3, P4);

	template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	value_t<R> call(std::string name, P1, P2, P3, P4, P5);

private:
	Serializer* call_(std::string name, const char* data, int len);

	template<typename R>
	value_t<R> net_call(Serializer& ds);

	template<typename F>
	void callproxy(F fun, Serializer* pr, const char* data, int len);

	template<typename F, typename S>
	void callproxy(F fun, S* s, Serializer* pr, const char* data, int len);

	// PROXY FUNCTION POINT
	template<typename R>
	void callproxy_(R(*func)(), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R()>(func), pr, data, len);
	}

	template<typename R, typename P1>
	void callproxy_(R(*func)(P1), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1)>(func), pr, data, len);
	}

	template<typename R, typename P1, typename P2>
	void callproxy_(R(*func)(P1, P2), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2)>(func), pr, data, len);
	}

	template<typename R, typename P1, typename P2, typename P3>
	void callproxy_(R(*func)(P1, P2, P3), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3)>(func), pr, data, len);
	}

	template<typename R, typename P1, typename P2, typename P3, typename P4>
	void callproxy_(R(*func)(P1, P2, P3, P4), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4)>(func), pr, data, len);
	}

	template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	void callproxy_(R(*func)(P1, P2, P3, P4, P5), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4, P5)>(func), pr, data, len);
	}

	// PROXY CLASS MEMBER
	template<typename R, typename C, typename S>
	void callproxy_(R(C::* func)(), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R()>(std::bind(func, s)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1>
	void callproxy_(R(C::* func)(P1), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1)>(std::bind(func, s, std::placeholders::_1)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1, typename P2>
	void callproxy_(R(C::* func)(P1, P2), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2)>(std::bind(func, s, std::placeholders::_1, std::placeholders::_2)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1, typename P2, typename P3>
	void callproxy_(R(C::* func)(P1, P2, P3), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3)>(std::bind(func, s, 
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1, typename P2, typename P3, typename P4>
	void callproxy_(R(C::* func)(P1, P2, P3, P4), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4)>(std::bind(func, s,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)), pr, data, len);
	}

	template<typename R, typename C, typename S, typename P1, typename P2, typename P3, typename P4, typename P5>
	void callproxy_(R(C::* func)(P1, P2, P3, P4, P5), S* s, Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(P1, P2, P3, P4, P5)>(std::bind(func, s,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)), pr, data, len);
	}

	// PORXY FUNCTIONAL
	template<typename R>
	void callproxy_(std::function<R()>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1>
	void callproxy_(std::function<R(P1)>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1, typename P2>
	void callproxy_(std::function<R(P1, P2)>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1, typename P2, typename P3>
	void callproxy_(std::function<R(P1, P2, P3)>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1, typename P2, typename P3, typename P4>
	void callproxy_(std::function<R(P1, P2, P3, P4)>, Serializer* pr, const char* data, int len);

	template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	void callproxy_(std::function<R(P1, P2, P3, P4, P5)>, Serializer* pr, const char* data, int len);

private:
	std::map<std::string, std::function<void(Serializer*, const char*, int)>> m_handlers;

	zmq::context_t m_context;
	zmq::socket_t* m_socket;

	rpc_err_code m_error_code;

	int m_role;
};

#include "buttonrpc_imp1.hpp"
