use std::sync::Mutex;
use serde_json::json;
use actix_web::{web, HttpResponse, Responder};
use super::super::{Error, MaaManager};
pub async fn create(maa_manager:web::Data<Mutex<MaaManager>>)-> Result<impl Responder, Error>{
    let id = maa_manager.lock().map_err(|_|Error::Internal)?.create();
    Ok(HttpResponse::Ok().json(json!({
        "id": id,
		"result":  0,
    })))
}
