use lazy_static::lazy_static;
use serde_json::Value;
use std::path::PathBuf;
use crate::CONFIG;
lazy_static! {
    static ref MSG_DB: sled::Db = {
        let mut p = PathBuf::new();
        p.push(CONFIG.database.path.clone());
        p.push("message");
        sled::open(p.as_os_str()).unwrap()
    };
}

#[derive(Debug)]
pub enum Error {
    Sled(sled::Error),
    SerdeJson(serde_json::Error),
    IVecNotLongEnough,
    InvalidUtf8String,
}

impl From<sled::Error> for Error {
    fn from(e: sled::Error) -> Self {
        Self::Sled(e)
    }
}

impl From<serde_json::Error> for Error {
    fn from(e: serde_json::Error) -> Self {
        Self::SerdeJson(e)
    }
}
#[derive(Debug)]
pub struct Msg {
    pub time: i64,
    pub type_: u32,
    pub uuid: String,
    pub body: String,
}

impl Msg {
    fn from_ivec(uuid: &str, ivec: &sled::IVec) -> Result<Self, Error> {
        if ivec.len() <= 12 {
            return Err(Error::IVecNotLongEnough);
        };
        let time = {
            let tmp: [u8; 8] = [
                ivec[0], ivec[1], ivec[2], ivec[3], ivec[4], ivec[5], ivec[6], ivec[7],
            ];
            i64::from_be_bytes(tmp)
        };
        let type_ = {
            let tmp: [u8; 4] = [ivec[8], ivec[9], ivec[10], ivec[11]];
            u32::from_be_bytes(tmp)
        };
        let body = String::from_utf8(ivec[12..].to_vec()).map_err(|_| Error::InvalidUtf8String)?;
        let msg = Msg {
            time,
            uuid: uuid.to_string(),
            type_,
            body,
        };
        Ok(msg)
    }
}
pub fn insert_msg(msg: &Msg) -> Result<u64, Error> {
    let uuid_tree = MSG_DB.open_tree(&msg.uuid)?;
    let id = MSG_DB.generate_id()?;
    let mut value = msg.time.to_be_bytes().to_vec();
    value.extend_from_slice(&msg.type_.to_be_bytes());
    value.extend_from_slice(msg.body.as_bytes());
    uuid_tree.insert(id.to_be_bytes(), value)?;
    Ok(id)
}
#[allow(dead_code)]
pub fn get_msg(uuid: &str, id: u64) -> Result<Option<Msg>, Error> {
    let uuid_tree = MSG_DB.open_tree(uuid)?;
    match uuid_tree.get(id.to_be_bytes())? {
        Some(value) => {
            let msg = Msg::from_ivec(uuid, &value)?;
            Ok(Some(msg))
        }
        None => Ok(None),
    }
}

pub fn get_all_uuid() -> Result<Vec<String>, Error> {
    let mut result = Vec::new();
    for i in MSG_DB.tree_names() {
        let s = String::from_utf8(i.to_vec()).map_err(|_| Error::InvalidUtf8String)?;
        if s == "__sled__default" {
            continue;
        }
        result.push(s);
    }
    Ok(result)
}
pub fn get_last_msg(uuid: &str, nums: usize) -> Result<Vec<Msg>, Error> {
    let uuid_tree = MSG_DB.open_tree(uuid)?;
    if uuid_tree.is_empty(){
        drop(uuid)?;
    }
    let mut iter = uuid_tree.iter();
    let mut result = Vec::new();
    for _ in 0..nums {
        match iter.next_back() {
            Some(Ok((_, v))) => {
                result.push(Msg::from_ivec(uuid, &v)?);
            }
            _ => break,
        }
    }
    return Ok(result);
}
pub fn drop(uuid: &str) -> Result<(), Error> {
    MSG_DB.drop_tree(uuid)?;
    Ok(())
}

pub fn drop_all() -> Result<(), Error> {
    for uuid in get_all_uuid()? {
        MSG_DB.drop_tree(uuid)?;
    }
    Ok(())
}
#[allow(unused_variables)]
#[allow(unused_must_use)]
pub unsafe extern "C" fn maa_store_callback(
    msg: std::os::raw::c_int,
    detail_json: *const ::std::os::raw::c_char,
    id: *mut ::std::os::raw::c_void,
) {
    std::panic::catch_unwind(|| {
        let body = std::ffi::CStr::from_ptr(detail_json)
            .to_str()
            .unwrap()
            .to_string();
        let now = chrono::Local::now().timestamp_millis();
        if let Ok(value) = serde_json::from_str::<Value>(&body) {
            if let Some(uuid) = value.get("uuid") {
                if let Some(uuid) = uuid.as_str() {
                    let msg = Msg {
                        time: now,
                        type_: msg as u32,
                        uuid: uuid.to_string(),
                        body: body,
                    };
                    insert_msg(&msg).unwrap();
                }
            }
        }
    });
}
