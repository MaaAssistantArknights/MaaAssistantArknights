use crate::database;

use super::super::Error;
use actix_web::{web, HttpResponse, Responder};
use serde::Deserialize;
use serde_json::json;

#[allow(dead_code)]
#[derive(Deserialize)]
pub struct Req {
    uuid: String,
}

pub async fn drop(req: web::Json<Req>) -> Result<impl Responder, Error> {
    database::msg::drop(&req.uuid).map_err(|_|Error::Internal)?;
    Ok(HttpResponse::Ok().json(json!({
        "result":  0,
    })))
}
