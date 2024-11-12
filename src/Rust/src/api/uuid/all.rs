use crate::database;
use super::super::Error;
use actix_web::{HttpResponse, Responder};
use serde_json::json;

///获取数据库内的uuid
pub async fn all() -> Result<impl Responder, Error> {
    Ok(HttpResponse::Ok().json(json!({
        "uuid": database::msg::get_all_uuid().map_err(|_|Error::Internal)?,
        "result":  0,
    })))
}