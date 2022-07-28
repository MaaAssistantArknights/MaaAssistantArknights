use std::sync::Mutex;
use super::super::{Error, MaaManager};
use actix_web::{HttpResponse, Responder, web};
use serde::Deserialize;
use serde_json::json;

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    id: i64,
    x: i32,
    y: i32

}
pub async fn click(req: web::Json<Req>, maa_manager:web::Data<Mutex<MaaManager>>) -> Result<impl Responder, Error> {
    let manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get(req.id).ok_or(Error::InstanceNotFound)?;
    maa.click(req.x, req.y)?;
    Ok(HttpResponse::Ok().json(json!({
        "result":  0,
    })))
}