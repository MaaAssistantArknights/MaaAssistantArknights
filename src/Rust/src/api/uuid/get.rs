use std::sync::Mutex;
use crate::api::MaaManager;
use super::super::Error;
use actix_web::{HttpResponse, Responder, web};
use serde::Deserialize;
use serde_json::json;

#[derive(Deserialize)]
pub struct Req {
    id: i64,
}
pub async fn get(req: web::Json<Req>,maa_manager:web::Data<Mutex<MaaManager>>) -> Result<impl Responder, Error> {
    let mut manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get_mut(req.id).ok_or(Error::InstanceNotFound)?;
    let uuid = maa.get_uuid()?;
    Ok(HttpResponse::Ok().json(json!({
        "uuid": uuid,
        "result":  0,
    })))
}
