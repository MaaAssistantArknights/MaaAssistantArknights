use std::sync::Mutex;
use serde_json::json;
use serde::Deserialize;
use actix_web::{web, HttpResponse, Responder};
use super::super::{Error, MaaManager};

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    id: i64,
}

pub async fn stop(req: web::Json<Req>, maa_manager:web::Data<Mutex<MaaManager>>)-> Result<impl Responder, Error>{
    let manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get(req.id).ok_or(Error::InstanceNotFound)?;
    maa.stop()?;
    Ok(HttpResponse::Ok().json(json!({
		"result":  0,
    })))
}