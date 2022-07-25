use actix_web::web;
mod click;
mod screenshot;
pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.route("/device/click", web::post().to(click::click));
    cfg.route("/device/screenshot", web::post().to(screenshot::screenshot));
}
