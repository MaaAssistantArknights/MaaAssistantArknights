use actix_web::web;
mod all;
mod get;
pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.route("/uuid/all", web::post().to(all::all));
    cfg.route("/uuid/get", web::post().to(get::get));
}
