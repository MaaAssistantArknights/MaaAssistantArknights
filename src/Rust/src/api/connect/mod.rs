use actix_web::web;
mod attach;
mod target;
pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.route("/connect/attach", web::post().to(attach::attach));
    cfg.route("/connect/target", web::post().to(target::target));
}