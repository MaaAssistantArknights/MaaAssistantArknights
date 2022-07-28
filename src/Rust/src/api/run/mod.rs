use actix_web::web;
mod start;
mod stop;
pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.route("/run/start", web::post().to(start::start));
    cfg.route("/run/stop", web::post().to(stop::stop));
}
