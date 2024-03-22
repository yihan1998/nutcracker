#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

// use std::os::raw::c_int;
use std::net::{SocketAddr, TcpListener, TcpStream, UdpSocket};
use std::io::{self, Write};

pub mod ffi {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

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

}
