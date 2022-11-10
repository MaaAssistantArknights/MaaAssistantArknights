use std::sync::Mutex;
use super::super::{Error, MaaManager};
use actix_web::{HttpResponse, Responder, web};
use serde::Deserialize;
use serde_json::{json, Value};

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    id: i64,
    types: String,
    params: Value,

}
pub async fn create(req: web::Json<Req>, maa_manager:web::Data<Mutex<MaaManager>>) -> Result<impl Responder, Error> {
    let mut manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get_mut(req.id).ok_or(Error::InstanceNotFound)?;
    let params = match req.params {
        Value::Null => "{}".to_string(),
        Value::Object(_) => req.params.to_string(),
        _=>return Err(Error::InvalidRequest)
    };
    let task_id = maa.create_task(&req.types, &params)?;
    Ok(HttpResponse::Ok().json(json!({
        "result":  0,
        "task_id": task_id
    })))
}