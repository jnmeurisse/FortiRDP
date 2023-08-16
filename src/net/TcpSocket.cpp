#include <openssl/bio.h>
#include "net/TcpSocket.h"

namespace net {
	TcpSocket::TcpSocket() : 
		TcpSocket(::BIO_new(::BIO_s_connect()))
	{
	}

	
	TcpSocket::TcpSocket(::BIO* bio) :
		Socket(bio)
	{
		DEBUG_CTOR(_logger, "TcpSocket");


		BIO_set_close(get_bio(), BIO_CLOSE);
		BIO_set_nbio(get_bio(), 1);
	}


	TcpSocket::~TcpSocket()
	{
		DEBUG_DTOR(_logger, "TcpSocket");
	}


	bool TcpSocket::set_nodelay(bool no_delay)
	{
		return ::BIO_set_tcp_ndelay(get_fd(), no_delay ? 1 : 0) == 1;
	}


	bool TcpSocket::do_connect(int timeout)
	{
		return BIO_do_connect_retry(get_bio(), timeout, 0) == 1;
	}

};
