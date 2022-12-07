use std::{collections::HashMap, ffi::c_void};
use actix_web::{web, HttpResponse, http::{StatusCode, header::ContentType}};
use serde_json::json;
use crate::{maa_sys::{Maa, self}, database};
mod instances;
mod connect;
mod message;
mod version;
mod device;
mod task;
mod uuid;
mod run;
pub fn config(cfg: &mut web::ServiceConfig) {
    instances::config(cfg);
    connect::config(cfg);
    message::config(cfg);
    version::config(cfg);
    device::config(cfg);
    task::config(cfg);
    uuid::config(cfg);
    run::config(cfg);
}
#[derive(Debug)]
pub enum Error {
    Internal,
    InstanceNotFound,
    InvalidRequest,
}

impl From<maa_sys::Error> for Error{
    fn from(_: maa_sys::Error) -> Self {
        Self::Internal
    }
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl actix_web::error::ResponseError for Error {
    fn error_response(&self) -> HttpResponse {
        let body =match self {
            Error::Internal => json!({
                "error":"内部错误"
            }),
            Error::InstanceNotFound => json!({
                "error":"实例不存在"
            }),
            Error::InvalidRequest => json!({
                "error":"无效的请求"
            }),
        };
        HttpResponse::build(self.status_code())
            .insert_header(ContentType::json())
            .json(body)
    }
    fn status_code(&self) -> StatusCode {
        match self {
            _ => StatusCode::OK,
        }
    }
}
 
pub struct MaaManager{
    pub instances:HashMap<i64, Maa>,
    id:i64
}

impl MaaManager {
    pub fn new()->Self{
        MaaManager { 
            instances: HashMap::new(), 
            id:0 
        }
    }
    pub fn create(&mut self)->i64{
        let id = self.gen_id();
        let maa = Maa::with_callback_and_custom_arg(Some(database::msg::maa_store_callback), id as *mut c_void);
        self.instances.insert(id, maa);
        id
    }
    pub fn get(&self, id:i64)->Option<&Maa>{
        self.instances.get(&id)
    }
    pub fn get_mut(&mut self, id:i64)->Option<&mut Maa>{
        self.instances.get_mut(&id)
    }
    pub fn get_target(&self, id:i64)->Option<String>{
        let maa = self.get(id)?;
        maa.get_target() 
    }
    pub fn delete(&mut self, id:i64)->Option<Maa>{
        let maa = self.instances.remove(&id);
        maa
    }
    pub fn get_all_id(&self)->Vec<i64>{
        self.instances.keys().map(|x|*x).collect()
    }
    pub fn gen_id(&mut self)->i64{
        self.id = self.id +1;
        return self.id
    }
}
unsafe impl Send for MaaManager{}