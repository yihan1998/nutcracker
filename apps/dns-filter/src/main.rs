use cacao;
use cacao::SocketType;

use std::io;
use std::net::{UdpSocket, SocketAddr};

fn callback_func(c: cacao::Context) -> Result<(), io::Error> {
//     let mut buf = [0u8; 1024];
//     let (len, src) = c.recv_from(&mut buf);
//     let mut msg = match Message::from_bytes(&buf[..len]) {
//         Ok(msg) => msg,
//         Err(e) => {
//             warn!("Failed to parse DNS query: {:?}", e);
//             continue;
//         },
//     };
//     c.send_to(&msg, src);
    Ok(())
}

fn main() -> Result<(), io::Error> {
    // Create a UDP socket
    let udp_socket = UdpSocket::bind("0.0.0.0:1234")?; // Bind to any available port
    let context = cacao::Context::new(SocketType::Udp(udp_socket));
    cacao::run(context, callback_func);
    Ok(())
}
