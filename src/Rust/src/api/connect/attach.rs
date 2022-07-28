use std::sync::Mutex;
use serde::Deserialize;
use serde_json::{json, Value};
use actix_web::{web, HttpResponse, Responder};
use super::super::{Error, MaaManager};

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    id: i64, 
    adb_path:String,
    target:String,
    config:Value
}
pub async fn attach(req: web::Json<Req>, maa_manager:web::Data<Mutex<MaaManager>>)-> Result<impl Responder, Error>{
    let mut manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get_mut(req.id).ok_or(Error::InstanceNotFound)?;
    if let Value::Null=req.config{
        maa.connect(&req.adb_path, &req.target, None)?;
    }else{
        maa.connect(&req.adb_path, &req.target, Some(&req.config.to_string()))?;
    }
    Ok(HttpResponse::Ok().json(json!({
		"result":  0,
    })))
}