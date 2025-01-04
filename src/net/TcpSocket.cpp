#include "net/TcpSocket.h"

namespace net {


	TcpSocket::TcpSocket() : 
		TcpSocket(::netctx_alloc())
	{
	}

	
	TcpSocket::TcpSocket(netctx* ctx) :
		Socket(ctx)
	{
		DEBUG_CTOR(_logger, "TcpSocket");
	}


	TcpSocket::~TcpSocket()
	{
		DEBUG_DTOR(_logger, "TcpSocket");

		// close the network context if not yet done
		close();
	}


	mbed_errnum TcpSocket::connect(const Endpoint& ep)
	{
		DEBUG_ENTER(_logger, "TcpSocket", "connect");

		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x enter TcpSocket:connect ep=%s", 
				std::addressof(this), 
				ep.to_string().c_str()
			);

		const std::string host{ ep.hostname() };
		const std::string port{ std::to_string(ep.port()) };
		mbed_errnum errnum;

		errnum = ::netctx_connect(get_ctx(), host.c_str(), port.c_str(), NETCTX_PROTO_TCP);
		if (errnum)
			goto terminate;

		// configure as a non blocking socket
		errnum = ::netctx_set_blocking(get_ctx(), true);
		if (errnum)
			goto terminate;

	terminate:
		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x leave TcpSocket::connect fd=%d rc=%d", 
				std::addressof(this), 
				get_fd(), 
				errnum
			);

		return errnum;
	}


	mbed_errnum TcpSocket::close()
	{
		DEBUG_ENTER(_logger, "TcpSocket", "close");

		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x enter TcpSocket::close fd=%d",
				std::addressof(this), 
				get_fd()
			);

		// netctx_close always succeed.
		::netctx_close(get_ctx());
		return 0;
	}


	mbed_errnum TcpSocket::set_nodelay(bool no_delay)
	{
		return ::netctx_set_nodelay(get_ctx(), no_delay);
	}


	netctx_poll_status TcpSocket::poll_rcv(uint32_t timeout)
	{
		return poll(true, false, timeout);
	}


	netctx_poll_status TcpSocket::poll_snd(uint32_t timeout)
	{
		return poll(false, true, timeout);
	}


	netctx_rcv_status TcpSocket::recv(unsigned char* buf, size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter Socket::recv buffer=%x len=%d", 
				std::addressof(this), 
				buf, 
				len
			);

		return ::netctx_recv(get_ctx(), buf, len);
	}


	netctx_snd_status TcpSocket::send(const unsigned char* buf, size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x enter Socket::send buffer=%x len=%d", 
				std::addressof(this), 
				buf, 
				len
			);

		return ::netctx_send(get_ctx(), buf, len);
	}


	netctx_rcv_status TcpSocket::read(unsigned char* buf, size_t len, Timer& timer)
	{
		netctx_rcv_status read_status = { NETCTX_RCV_ERROR, 0, 0 };

		do {
			const netctx_rcv_status rcv_status = recv(buf, len);

			buf += rcv_status.rbytes;
			len -= rcv_status.rbytes;

			read_status.status_code = rcv_status.status_code;
			read_status.rbytes += rcv_status.rbytes;
			read_status.errnum = rcv_status.errnum;

			if (rcv_status.status_code == NETCTX_RCV_RETRY) {
				const netctx_poll_status poll_status = poll_rcv(timer.remaining_delay());

				if (poll_status.status_code != NETCTX_POLL_OK) {
					read_status.status_code = NETCTX_RCV_ERROR;
					read_status.errnum = poll_status.errnum;
				}
			}
		} while (len >= 0 && read_status.status_code == NETCTX_RCV_RETRY);

		return read_status;
	}


	netctx_snd_status TcpSocket::write(const unsigned char* buf, size_t len, Timer& timer)
	{
		netctx_snd_status write_status = { NETCTX_SND_ERROR, 0, 0 };

		do {
			const netctx_snd_status snd_status = send(buf, len);

			buf += snd_status.sbytes;
			len -= snd_status.sbytes;

			write_status.status_code = snd_status.status_code;
			write_status.sbytes += snd_status.sbytes;
			write_status.errnum = snd_status.errnum;

			if (snd_status.status_code == NETCTX_SND_RETRY) {
				const netctx_poll_status poll_status = poll_snd(timer.remaining_delay());

				if (poll_status.status_code != NETCTX_POLL_OK) {
					write_status.status_code = NETCTX_SND_ERROR;
					write_status.errnum = poll_status.errnum;
				}
			}
		} while (len >= 0 && write_status.status_code == NETCTX_SND_RETRY);

		return write_status;
	}


	int net::TcpSocket::get_fd() const noexcept
	{
		return ::netctx_getfd(get_ctx());
	}

	
	netctx_poll_status TcpSocket::poll(bool read, bool write, uint32_t timeout)
	{
		netctx_poll_mode mode;
		mode.read = read ? 1 : 0;
		mode.write = write ? 1 : 0;

		return ::netctx_poll(get_ctx(), mode, timeout);
	}
};
