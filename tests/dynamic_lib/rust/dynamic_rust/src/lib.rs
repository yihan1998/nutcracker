fn print() {
    println!("Hello from print in Rust library!")
}

#[no_mangle]
pub extern "C" fn callback_func() {
    print()
}