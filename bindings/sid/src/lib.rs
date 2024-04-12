#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use std::os::raw::c_int;
use std::ffi::CString;
use std::os::raw::c_char;


fn convert_error(ret: c_int) -> Result<(), i32> {
    if ret == 0 {
        Ok(())
    } else {
        Err(ret as i32)
    }
}
pub mod ffi {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

pub fn sid_object__open_and_load(path :String) -> i32 {
    let object_path = CString::new(path).expect("CString::new failed");
    let object_path_ptr: *const c_char = object_path.as_ptr();
    unsafe { ffi::sid_object__open_and_load(object_path_ptr) }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = sid_object__open_and_load(1,1,1);
        assert_eq!(result, Err(-1));
    }
}
