use std::fs;
use serde::{Serialize, Deserialize};
use lazy_static::lazy_static;
lazy_static! {
    pub static ref CONFIG: Config = {
        let s = fs::read("./server_config.json").unwrap();
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
