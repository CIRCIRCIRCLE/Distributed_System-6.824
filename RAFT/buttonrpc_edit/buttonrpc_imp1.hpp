#pragma once

#include "buttonrpc.hpp"

template<typename F>
void buttonrpc::bind( std::string name, F func )
{
	m_handlers[name] = std::bind(&buttonrpc::callproxy<F>, this, func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

template<typename F, typename S>
inline void buttonrpc::bind(std::string name, F func, S* s)
{
	m_handlers[name] = std::bind(&buttonrpc::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

template<typename F>
void buttonrpc::callproxy( F fun, Serializer* pr, const char* data, int len )
{
	callproxy_(fun, pr, data, len);
}

template<typename F, typename S>
inline void buttonrpc::callproxy(F fun, S * s, Serializer * pr, const char * data, int len)
{
	callproxy_(fun, s, pr, data, len);
}

// help call return value type is void function
template<typename R, typename F>
typename std::enable_if<std::is_same<R, void>::value, typename type_xx<R>::type >::type call_helper(F f) {
	f();
	return 0;
}

template<typename R, typename F>
typename std::enable_if<!std::is_same<R, void>::value, typename type_xx<R>::type >::type call_helper(F f) {
	return f();
}

template<typename R>
void buttonrpc::callproxy_(std::function<R()> func, Serializer* pr, const char* data, int len)
{
	typename type_xx<R>::type r = call_helper<R>(std::bind(func));

	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1>
void buttonrpc::callproxy_(std::function<R(P1)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1;
	ds >> p1;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1));

	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1, typename P2>
void buttonrpc::callproxy_(std::function<R(P1, P2)> func, Serializer* pr, const char* data, int len )
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2;
	ds >> p1 >> p2;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2));
	
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1, typename P2, typename P3>
void buttonrpc::callproxy_(std::function<R(P1, P2, P3)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2; P3 p3;
	ds >> p1 >> p2 >> p3;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2, p3));
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1, typename P2, typename P3, typename P4>
void buttonrpc::callproxy_(std::function<R(P1, P2, P3, P4)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2; P3 p3; P4 p4;
	ds >> p1 >> p2 >> p3 >> p4;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2, p3, p4));
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
void buttonrpc::callproxy_(std::function<R(P1, P2, P3, P4, P5)> func, Serializer* pr, const char* data, int len)
{
	Serializer ds(StreamBuffer(data, len));
	P1 p1; P2 p2; P3 p3; P4 p4; P5 p5;
	ds >> p1 >> p2 >> p3 >> p4 >> p5;
	typename type_xx<R>::type r = call_helper<R>(std::bind(func, p1, p2, p3, p4, p5));
	value_t<R> val;
	val.set_code(RPC_ERR_SUCCESS);
	val.set_val(r);
	(*pr) << val;
}

template<typename R>
inline buttonrpc::value_t<R> buttonrpc::net_call(Serializer& ds)
{
	zmq::message_t request(ds.size() + 1);
	memcpy(request.data(), ds.data(), ds.size());
	if (m_error_code != RPC_ERR_RECV_TIMEOUT) {
		send(request);
	}
	zmq::message_t reply;
	recv(reply);
	value_t<R> val;
	if (reply.size() == 0) {
		// timeout
		m_error_code = RPC_ERR_RECV_TIMEOUT;
		val.set_code(RPC_ERR_RECV_TIMEOUT);
		val.set_msg("recv timeout");
		return val;
	}
	m_error_code = RPC_ERR_SUCCESS;
	ds.clear();
	ds.write_raw_data((char*)reply.data(), reply.size());
	ds.reset();

	ds >> val;
	return val;
}

template<typename R>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name)
{
	Serializer ds;
	ds << name;
	return net_call<R>(ds);
}

template<typename R, typename P1>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1)
{
	Serializer ds;
	ds << name << p1;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2>
inline buttonrpc::value_t<R> buttonrpc::call( std::string name, P1 p1, P2 p2 )
{
	Serializer ds;
	ds << name << p1 << p2;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2, typename P3>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1, P2 p2, P3 p3)
{
	Serializer ds;
	ds << name << p1 << p2 << p3;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2, typename P3, typename P4>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1, P2 p2, P3 p3, P4 p4)
{
	Serializer ds;
	ds << name << p1 << p2 << p3 << p4;
	return net_call<R>(ds);
}

template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
inline buttonrpc::value_t<R> buttonrpc::call(std::string name, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
{
	Serializer ds;
	ds << name << p1 << p2 << p3 << p4 << p5;
	return net_call<R>(ds);
}
