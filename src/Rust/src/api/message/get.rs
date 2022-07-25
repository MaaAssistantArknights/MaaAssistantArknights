use crate::database;

use super::super::Error;
use actix_web::{web, HttpResponse, Responder};
use serde::Deserialize;
use serde_json::{json, Value};

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    uuid: String,
    nums: Option<i64>,
}

pub async fn get(req: web::Json<Req>) -> Result<impl Responder, Error> {
    let nums = req.nums.unwrap_or(i64::MAX);
    let msgs = database::msg::get_last_msg(&req.uuid, nums as usize).map_err(|_| Error::Internal)?;
    let mut ret: Vec<Value> = Vec::new();
    for x in msgs {
        ret.push(json!({
            "time":x.time,
            "body":serde_json::from_str::<Value>(&x.body).map_err(|_|Error::Internal)?,
            "type":x.type_,
        }))
    }
    Ok(HttpResponse::Ok().json(json!({
        "msgs": ret,
        "result":  0,
    })))
}
