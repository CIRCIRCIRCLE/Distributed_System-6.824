#include "buttonrpc.hpp"
#include <iostream>
#include <stdexcept>

buttonrpc::buttonrpc() : m_context(1){ 
	m_error_code = RPC_ERR_SUCCESS;
}

buttonrpc::~buttonrpc(){ 
	m_socket->close();
	delete m_socket;
	m_context.close();
}

// network
void buttonrpc::as_client( std::string ip, int port )
{
	m_role = RPC_CLIENT;
	m_socket = new zmq::socket_t(m_context, ZMQ_REQ);
	ostringstream os;
	os << "tcp://" << ip << ":" << port;
	m_socket->connect (os.str());
}

void buttonrpc::as_server( int port )
{
	m_role = RPC_SERVER;
	m_socket = new zmq::socket_t(m_context, ZMQ_REP);
	ostringstream os;
	os << "tcp://*:" << port;
	m_socket->bind (os.str());
}

void buttonrpc::send( zmq::message_t& data )
{
	m_socket->send(data);
}

void buttonrpc::recv( zmq::message_t& data )
{
	m_socket->recv(&data);
}

inline void buttonrpc::set_timeout(uint32_t ms)
{
	// only client can set
	if (m_role == RPC_CLIENT) {
		m_socket->setsockopt(ZMQ_RCVTIMEO, ms);
	}
}

void buttonrpc::run()
{
	// only server can call
	if (m_role != RPC_SERVER) {
		return;
	}
	while (1){
		zmq::message_t data;
		recv(data);
		StreamBuffer iodev((char*)data.data(), data.size());
		Serializer ds(iodev);

		std::string funname;
		ds >> funname;
		Serializer* r = call_(funname, ds.current(), ds.size()- funname.size());

		zmq::message_t retmsg (r->size());
		memcpy (retmsg.data (), r->data(), r->size());
		send(retmsg);
		delete r;
	}
}


Serializer* buttonrpc::call_(std::string name, const char* data, int len)
{
	Serializer* ds = new Serializer();
	if (m_handlers.find(name) == m_handlers.end()) {
		(*ds) << value_t<int>::code_type(RPC_ERR_FUNCTIION_NOT_BIND);
		(*ds) << value_t<int>::msg_type("function not bind: " + name);
		return ds;
	}
	auto fun = m_handlers[name];
	fun(ds, data, len);
	ds->reset();
	return ds;
}