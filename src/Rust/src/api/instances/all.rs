use std::sync::Mutex;
use serde_json::json;
use actix_web::{web, HttpResponse, Responder};
use super::super::{Error, MaaManager};
pub async fn all(maa_manager:web::Data<Mutex<MaaManager>>)-> Result<impl Responder, Error>{
    let instances = maa_manager.lock().map_err(|_|Error::Internal)?.get_all_id();
    Ok(HttpResponse::Ok().json(json!({
        "instances": instances,
		"result":  0,
    })))
}
