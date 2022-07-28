use std::sync::Mutex;
use super::super::{Error, MaaManager};
use actix_web::{HttpResponse, Responder, web};
use serde::Deserialize;

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    id: i64,
}
pub async fn screenshot(req: web::Json<Req>, maa_manager:web::Data<Mutex<MaaManager>>) -> Result<impl Responder, Error> {
    let manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get(req.id).ok_or(Error::InstanceNotFound)?;
    let body = maa.screenshot()?;
    Ok(HttpResponse::Ok().body(body))
}