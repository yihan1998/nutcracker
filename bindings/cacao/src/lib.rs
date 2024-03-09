#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

// use std::os::raw::c_int;
use std::net::{SocketAddr, TcpListener, TcpStream, UdpSocket};
use std::io::{self, Write};

// fn convert_error(ret: c_int) -> Result<(), i32> {
//     if ret == 0 {
//         Ok(())
//     } else {
//         Err(ret as i32)
//     }
// }

// pub mod ffi {
//     include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
// }

pub enum SocketType {
    Tcp(TcpStream),
    Udp(UdpSocket),
}

pub struct Context {
    pub socket: SocketType,
}

impl Context {
    pub fn new(socket: SocketType) -> Self {
        Context { socket }
    }

    pub fn send(&mut self, data: &[u8], addr: Option<SocketAddr>) -> std::io::Result<()> {
        match &mut self.socket {
            SocketType::Tcp(tcp_stream) => tcp_stream.write_all(data),
            SocketType::Udp(udp_socket) => {
                if let Some(address) = addr {
                    udp_socket.send_to(data, address)?;
                } else {
                    return Err(io::Error::new(io::ErrorKind::Other, "UDP address not specified"));
                }
                Ok(())
            }
        }
    }
}

pub fn run(_c: Context, _f: fn(c: Context) -> Result<(), io::Error> ) {

}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_new_context_with_udp() {
        // Set up a UDP socket
        let udp_socket = UdpSocket::bind("0.0.0.0:1234").unwrap();
        let context = Context::new(SocketType::Udp(udp_socket));
    }

    #[test]
    fn test_new_context_with_tcp() {
        // Set up a TCP listener
        let listener = TcpListener::bind("0.0.0.0:1234").unwrap();
        let addr = listener.local_addr().unwrap();
        let tcp_stream = std::net::TcpStream::connect(addr).unwrap();
        let context = Context::new(SocketType::Tcp(tcp_stream));
    }
}
