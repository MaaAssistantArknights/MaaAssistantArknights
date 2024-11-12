use std::{sync::Mutex, collections::HashMap};
use super::super::{Error, MaaManager};
use actix_web::{HttpResponse, Responder, web};
use serde::Deserialize;
use serde_json::json;

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    id: i64,

}
pub async fn all(req: web::Json<Req>, maa_manager:web::Data<Mutex<MaaManager>>) -> Result<impl Responder, Error> {
    let mut manager = maa_manager.lock().map_err(|_|Error::Internal)?;
    let maa = manager.get_mut(req.id).ok_or(Error::InstanceNotFound)?;
    let tasks = {
        let mut tmp = HashMap::new();
        for (k, v) in maa.get_tasks()?{
            tmp.insert(k.to_string(), json!({
                "type": v.type_,
                "params": v.params
            }));
        };
        tmp
    };
    Ok(HttpResponse::Ok().json(json!({
        "tasks":tasks,
        "result":  0,
    })))
}