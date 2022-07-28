use actix_web::web;
mod create;
mod set;
mod all;
pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.route("/task/create", web::post().to(create::create));
    cfg.route("/task/set", web::post().to(set::set));
    cfg.route("/task/all", web::post().to(all::all));
}
