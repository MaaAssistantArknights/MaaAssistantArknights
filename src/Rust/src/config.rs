use lazy_static::lazy_static;
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

lazy_static! {
    pub static ref CONFIG: Config = {
        let config_file = Path::new("./server_config.json");
        if !config_file.exists() {
            let default_config = include_str!("../server_config.json");
            fs::write(config_file,default_config).unwrap();
        }
        let s = fs::read(config_file).unwrap();
        let r: Config = serde_json::from_slice(&s).unwrap();
        r
    };
}

#[derive(Serialize, Deserialize)]
pub struct Config {
    #[serde(rename = "server")]
    pub server: Server,

    #[serde(rename = "database")]
    pub database: Database,

    #[serde(rename = "resource")]
    pub resource: Resource,
}

#[derive(Serialize, Deserialize)]
pub struct Database {
    #[serde(rename = "path")]
    pub path: String,

    #[serde(rename = "drop_on_start_up")]
    pub drop_on_start_up: bool,
}

#[derive(Serialize, Deserialize)]
pub struct Server {
    #[serde(rename = "address")]
    pub address: String,

    #[serde(rename = "port")]
    pub port: u16,
}

#[derive(Serialize, Deserialize)]
pub struct Resource {
    #[serde(rename = "path")]
    pub path: String,
}
