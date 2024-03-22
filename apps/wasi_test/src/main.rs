extern crate regex;
use regex::Regex;

pub fn test() {
    println!("Bonne soirée!")
}

fn main() -> std::io::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    println!("Received arguments: {:?}", args);

    unsafe {
        // Attempt to parse the string as a usize (pointer-sized integer).
        // This assumes the pointer is passed as a hexadecimal string.
        if let Ok(address) = usize::from_str_radix(&args[1].trim_start_matches("0x"), 16) {
            // Cast the usize to a raw pointer to i32.
            let ptr = address as *const i32;
            
            // Dereference the pointer to read the value.
            // This will crash if the address is invalid or inaccessible.
            println!("Value at address 0x{:X} is: {}", address, *ptr);
        } else {
            println!("Invalid pointer address.");
        }
    }

    let text = "The current date is 2023-03-15.";
    let date_re = Regex::new(r"\d{4}-\d{2}-\d{2}").unwrap();

    match date_re.find(text) {
        Some(matched) => println!("Found a date: {}", matched.as_str()),
        None => println!("No dates found"),
    }
    test();
    Ok(())
}