use std::{
    collections::{HashMap, HashSet},
    ffi::c_void,
};

include!("./bind.rs");

#[derive(Debug)]
pub enum Error {
    Unknown,
    TooLargeAlloc,
    Null,
    Utf8,
}

impl From<std::ffi::NulError> for Error {
    fn from(_: std::ffi::NulError) -> Self {
        Self::Null
    }
}

impl From<std::str::Utf8Error> for Error {
    fn from(_: std::str::Utf8Error) -> Self {
        Self::Utf8
    }
}
#[derive(Debug, Clone)]
pub struct Task {
    pub id: i32,
    pub type_: String,
    pub params: String,
}

#[derive(Debug)]
pub struct Maa {
    handle: AsstHandle,
    uuid: Option<String>,
    target: Option<String>,
    tasks: HashMap<i32, Task>,
}

impl Maa {
    #[allow(dead_code)]
    pub fn new() -> Self {
        unsafe {
            Maa {
                handle: AsstCreate(),
                uuid: None,
                target: None,
                tasks: HashMap::new(),
            }
        }
    }
    pub fn get_null_size() -> u64 {
        unsafe { AsstGetNullSize() }
    }
    pub fn with_callback_and_custom_arg(
        call_back: AsstApiCallback,
        custom_arg: *mut std::os::raw::c_void,
    ) -> Self {
        unsafe {
            Maa {
                handle: AsstCreateEx(call_back, custom_arg),
                uuid: None,
                target: None,
                tasks: HashMap::new(),
            }
        }
    }
    #[allow(dead_code)]
    pub fn with_callback(call_back: AsstApiCallback) -> Self {
        Self::with_callback_and_custom_arg(call_back, 0 as *mut std::os::raw::c_void)
    }
    #[allow(dead_code)]
    pub unsafe extern "C" fn default_callback(
        msg: std::os::raw::c_int,
        detail_json: *const ::std::os::raw::c_char,
        _: *mut ::std::os::raw::c_void,
    ) {
        println!(
            "msg:{}: {}",
            msg,
            std::ffi::CStr::from_ptr(detail_json)
                .to_str()
                .unwrap()
                .to_string()
        );
    }
    pub fn load_resource(path: &str) -> Result<(), Error> {
        let ret: AsstBool;
        unsafe {
            let path = std::ffi::CString::new(path.to_string())?;
            ret = AsstLoadResource(path.as_ptr());
        };
        match ret {
            1 => Ok(()),
            _ => Err(Error::Unknown),
        }
    }
    pub fn get_version() -> Result<String, Error> {
        unsafe {
            let c = AsstGetVersion();
            let ret = std::ffi::CStr::from_ptr(c).to_str()?.to_string();
            Ok(ret)
        }
    }
    #[allow(dead_code)]
    pub fn set_static_option(option: AsstStaticOptionKey, value: &str) -> Result<(), Error> {
        let c_option_value = std::ffi::CString::new(value)?;
        unsafe {
            if AsstSetStaticOption(option, c_option_value.as_ptr()) == 1 {
                Ok(())
            } else {
                Err(Error::Unknown)
            }
        }
    }
    #[allow(dead_code)]
    pub fn set_working_directory(path: &str) -> Result<(), Error> {
        let c_path = std::ffi::CString::new(path)?;
        unsafe {
            if AsstSetUserDir(c_path.as_ptr()) == 1 {
                Ok(())
            } else {
                Err(Error::Unknown)
            }
        }
    }
    #[allow(dead_code)]
    pub fn set_option(&mut self, option: AsstInstanceOptionKey, value: &str) -> Result<(), Error> {
        let c_option_value = std::ffi::CString::new(value)?;
        unsafe {
            if AsstSetInstanceOption(self.handle, option, c_option_value.as_ptr()) == 1 {
                Ok(())
            } else {
                Err(Error::Unknown)
            }
        }
    }
    pub fn connect(
        &mut self,
        adb_path: &str,
        address: &str,
        config: Option<&str>,
    ) -> Result<i32, Error> {
        unsafe {
            let c_adb_path = std::ffi::CString::new(adb_path)?;
            let c_address = std::ffi::CString::new(address)?;
            let c_cfg_ptr = match config {
                Some(cfg) => std::ffi::CString::new(cfg)?.as_ptr(),
                None => 0 as *const std::os::raw::c_char,
            };
            let ret = AsstAsyncConnect(
                self.handle,
                c_adb_path.as_ptr(),
                c_address.as_ptr(),
                c_cfg_ptr,
                1,
            );
            if ret != 0 {
                self.target = Some(address.to_string());
                Ok(ret)
            } else {
                Err(Error::Unknown)
            }
        }
    }

    #[deprecated]
    #[allow(dead_code)]
    pub fn connect_legacy(
        &mut self,
        adb_path: &str,
        address: &str,
        config: Option<&str>,
    ) -> Result<(), Error> {
        let c_adb_path = std::ffi::CString::new(adb_path)?;
        let c_address = std::ffi::CString::new(address)?;
        let c_cfg_ptr = match config {
            Some(cfg) => std::ffi::CString::new(cfg)?.as_ptr(),
            None => 0 as *const std::os::raw::c_char,
        };
        unsafe {
            let ret = AsstConnect(
                self.handle,
                c_adb_path.as_ptr(),
                c_address.as_ptr(),
                c_cfg_ptr,
            );
            if ret == 1 {
                Ok(())
            } else {
                Err(Error::Unknown)
            }
        }
    }
    #[allow(dead_code)]
    pub fn running(&self) -> bool {
        unsafe { AsstRunning(self.handle) == 1 }
    }
    pub fn click(&self, x: i32, y: i32) -> Result<i32, Error> {
        unsafe {
            let ret = AsstAsyncClick(self.handle, x, y, 0);
            if ret != 0 {
                Ok(ret)
            } else {
                Err(Error::Unknown)
            }
        }
    }
    pub fn screenshot(&self) -> Result<Vec<u8>, Error> {
        unsafe {
            let mut buff_size = 2 * 1920 * 1080 * 4;
            loop {
                if buff_size > 10 * 1920 * 1080 * 4 {
                    return Err(Error::TooLargeAlloc);
                }
                let mut buff: Vec<u8> = Vec::with_capacity(buff_size);
                let data_size = AsstGetImage(
                    self.handle,
                    buff.as_mut_ptr() as *mut c_void,
                    buff_size as u64,
                );
                if data_size == Self::get_null_size() {
                    buff_size = 2 * buff_size;
                    continue;
                }
                buff.set_len(data_size as usize);
                buff.resize(data_size as usize, 0);
                return Ok(buff);
            }
        }
    }
    #[allow(dead_code)]
    pub fn take_screenshot(&mut self) -> Result<(), Error> {
        unsafe {
            match AsstAsyncScreencap(self.handle, 1) {
                0 => Err(Error::Unknown),
                _ => Ok(()),
            }
        }
    }
    pub fn create_task(&mut self, type_: &str, params: &str) -> Result<AsstTaskId, Error> {
        unsafe {
            let c_type = std::ffi::CString::new(type_)?;
            let c_params = std::ffi::CString::new(params)?;
            let task_id = AsstAppendTask(self.handle, c_type.as_ptr(), c_params.as_ptr());
            self.tasks.insert(
                task_id,
                Task {
                    id: task_id,
                    type_: type_.to_string(),
                    params: params.to_string(),
                },
            );
            Ok(task_id)
        }
    }
    pub fn set_task(&self, id: i32, params: &str) -> Result<(), Error> {
        unsafe {
            let c_params = std::ffi::CString::new(params)?;
            match AsstSetTaskParams(self.handle, id, c_params.as_ptr()) {
                1 => Ok(()),
                _ => Err(Error::Unknown),
            }
        }
    }
    pub fn get_uuid(&mut self) -> Result<String, Error> {
        if let Some(uuid) = self.uuid.clone() {
            return Ok(uuid);
        };
        unsafe {
            let mut buff_size = 1024;
            loop {
                if buff_size > 1024 * 1024 {
                    return Err(Error::TooLargeAlloc);
                }
                let mut buff: Vec<u8> = Vec::with_capacity(buff_size);
                let data_size =
                    AsstGetUUID(self.handle, buff.as_mut_ptr() as *mut i8, buff_size as u64);
                if data_size == Self::get_null_size() {
                    buff_size = 2 * buff_size;
                    continue;
                }
                buff.set_len(data_size as usize);
                let ret = String::from_utf8_lossy(&buff).to_string();
                self.uuid = Some(ret.clone());
                return Ok(ret);
            }
        }
    }
    pub fn get_target(&self) -> Option<String> {
        return self.target.clone();
    }
    pub fn get_tasks(&mut self) -> Result<&HashMap<i32, Task>, Error> {
        unsafe {
            let mut buff_size = 1024;
            loop {
                if buff_size > 1024 * 1024 {
                    return Err(Error::TooLargeAlloc);
                }
                let mut buff: Vec<i32> = Vec::with_capacity(buff_size);
                let data_size =
                    AsstGetTasksList(self.handle, buff.as_mut_ptr() as *mut i32, buff_size as u64);
                if data_size == Self::get_null_size() {
                    buff_size = 2 * buff_size;
                    continue;
                }
                buff.set_len(data_size as usize);
                buff.resize(data_size as usize, 0);
                let mut task_ids = HashSet::with_capacity(buff.len());
                for i in buff {
                    task_ids.insert(i);
                }
                self.tasks.retain(|k, _| task_ids.contains(k));
                return Ok(&self.tasks);
            }
        }
    }
    pub fn start(&self) -> Result<(), Error> {
        unsafe {
            match AsstStart(self.handle) {
                1 => Ok(()),
                _ => Err(Error::Unknown),
            }
        }
    }
    pub fn stop(&self) -> Result<(), Error> {
        unsafe {
            match AsstStop(self.handle) {
                1 => Ok(()),
                _ => Err(Error::Unknown),
            }
        }
    }
    #[allow(dead_code)]
    pub fn log(level_str: &str, message: &str) -> Result<(), Error> {
        let c_level_str = std::ffi::CString::new(level_str)?;
        let c_message = std::ffi::CString::new(message)?;
        unsafe {
            AsstLog(c_level_str.as_ptr(), c_message.as_ptr());
        }
        Ok(())
    }
}

impl Drop for Maa {
    fn drop(&mut self) {
        unsafe { AsstDestroy(self.handle) }
    }
}
