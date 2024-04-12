use std::env;
use std::path::PathBuf;

fn main() {
    // Tell cargo to invalidate the built crate whenever the wrapper changes
    println!("cargo:rerun-if-changed=../../sid/sid.h");

    // Use the `cc` crate to compile your C library
    cc::Build::new()
        .file("../../sid/sid.c")
        .compile("libsid.a");

    // Generate bindings
    let bindings = bindgen::Builder::default()
        .header("../../sid/sid.h")
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    // Write the bindings to the `src/bindings.rs` file.
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
