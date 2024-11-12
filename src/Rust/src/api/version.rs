use crate::{maa_sys::Maa, SERVER_VERSION};
use super::Error;
use actix_web::{HttpResponse, Responder, web};
use serde_json::json;

pub async fn version() -> Result<impl Responder, Error> {
    let core_version = Maa::get_version()?;
    Ok(HttpResponse::Ok().json(json!({
        "core":core_version,
        "server":SERVER_VERSION,
        "result":  0,
    })))
}

pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.route("/version", web::post().to(version));
}
