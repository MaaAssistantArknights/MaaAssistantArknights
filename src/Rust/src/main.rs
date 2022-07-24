mod maa_sys;
mod database;
mod api;
mod config;
use config::CONFIG;
use std::sync::Mutex;
use maa_sys::Maa;
use actix_web::{middleware, rt, App, HttpServer, web};

const SERVER_VERSION:&str="v0.0.1";
fn main() -> std::io::Result<()> {
    Maa::load_resource(&CONFIG.resource.path).unwrap();
    if CONFIG.database.drop_on_start_up{
        database::msg::drop_all().unwrap();
    }
    tracing_subscriber::fmt()
        .with_max_level(tracing::Level::INFO)
        .init();
    let maa_manager = web::Data::new(Mutex::new(api::MaaManager::new()));
    rt::System::new().block_on(async {
        HttpServer::new(move|| {
            App::new()
                .app_data(maa_manager.clone())
                .wrap(middleware::Logger::default())
                .configure(api::config)
        })
        .bind((CONFIG.server.address.clone(), CONFIG.server.port))?
        .run()
        .await
    })
}