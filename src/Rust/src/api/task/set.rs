use std::sync::Mutex;
use super::super::{Error, MaaManager};
use actix_web::{HttpResponse, Responder, web};
use serde::Deserialize;
use serde_json::{json, Value};

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    id: i64,
    task_id: i32,
    params: Value,

}
pub async fn set(req: web::Json<Req>, maa_manager:web::Data<Mutex<MaaManager>>) -> Result<impl Responder, Error> {
    let manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get(req.id).ok_or(Error::InstanceNotFound)?;
    let params = match req.params {
        Value::Null => "{}".to_string(),
        Value::Object(_) => req.params.to_string(),
        _=>return Err(Error::InvalidRequest)
    };
    maa.set_task(req.task_id, &params)?;
    Ok(HttpResponse::Ok().json(json!({
        "result":  0,
    })))
}