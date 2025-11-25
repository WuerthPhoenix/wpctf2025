mod session_auth;
mod auth;
mod hosts;
mod proxy_config;

use dotenv::dotenv;
use proxy_config::ProxyConfig;
use rocket::serde::json::{json, Json, Value};
use rocket::{catch, catchers, launch, routes, Request, Rocket, Build, fs::FileServer, get, response::Redirect};

/// Root redirect to login page
#[get("/")]
pub fn index() -> Redirect {
    Redirect::to("/static/login.html")
}

#[launch]
async fn rocket() -> Rocket<Build> {
    if let Err(err) = dotenv() {
        eprintln!(".env file not found, unreadable or unparseable: {err:?}")
    }

    let config = ProxyConfig::new_from_env();

    rocket::build()
        .manage(config)
        .register("/", catchers![default_catcher])
        .mount("/", routes![
            index,
            auth::login,
            auth::logout,
            auth::me,
            hosts::get_hosts
        ])
        .mount("/static", FileServer::from("/app/static"))
}

#[catch(default)]
pub fn default_catcher(status: rocket::http::Status, _req: &Request) -> Json<Value> {
    Json(json!({
        "error": "Request failed",
        "status": status.code
    }))
}
