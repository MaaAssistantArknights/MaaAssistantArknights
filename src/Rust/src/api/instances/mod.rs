use actix_web::web;
mod create;
mod delete;
mod all;
pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.route("/instance/create", web::post().to(create::create));
    cfg.route("/instance/delete", web::post().to(delete::delete));
    cfg.route("/instance/all", web::post().to(all::all));
}