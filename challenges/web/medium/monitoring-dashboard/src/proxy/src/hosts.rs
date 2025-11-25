use rocket::{get, State};
use std::collections::HashMap;
use rocket::serde::json::Value;
use rocket::http::Status;
use serde::{Deserialize, Serialize};

use crate::session_auth::AuthenticatedUser;
use crate::proxy_config::ProxyConfig;

#[derive(Deserialize, Serialize)]
struct IcingaResponse {
    results: Vec<IcingaHost>,
    #[serde(flatten)]
    other: HashMap<String, Value>,
}

#[derive(Deserialize, Serialize)]
struct IcingaHost {
    attrs: HostAttrs,
    #[serde(flatten)]
    other: HashMap<String, Value>,
}

#[derive(Deserialize, Serialize)]
struct HostAttrs {
    groups: Vec<String>,
    #[serde(flatten)]
    other: HashMap<String, Value>,
}

async fn get_icinga_hosts(
    config: &ProxyConfig,
    query_params: Option<HashMap<String, String>>,
) -> Result<String, Status> {
    let client = reqwest::Client::builder()
        .connect_timeout(config.external_connection_timeout)
        .danger_accept_invalid_certs(true)
        .danger_accept_invalid_hostnames(true)
        .build()
        .map_err(|_| Status::ServiceUnavailable)?;

    let url = format!("{}/v1/objects/hosts", config.icinga2_endpoint);
    println!("Request: GET {url}");

    let mut builder = client.get(&url);
    if let Some(params) = query_params {
        builder = builder.query(&params);
    }

    let request = builder
        .header("Accept", "application/json")
        .basic_auth(&config.icinga2_username, Some(&config.icinga2_password))
        .build()
        .map_err(|_| Status::ServiceUnavailable)?;

    let response = client.execute(request).await
        .map_err(|_| Status::ServiceUnavailable)?;

    if response.status().is_success() {
        let text = response.text().await.map_err(|_| Status::ServiceUnavailable)?;
        println!("Upstream Response: {text}");
        Ok(text)
    } else {
        let status_code = response.status().as_u16();
        let body = response.text().await.map_err(|_| Status::ServiceUnavailable)?;
        eprintln!("Response ({}): {}", status_code, body);
        Err(Status::new(status_code))
    }
}

/// Get hosts with any query parameters, filtered by user hostgroup
/// Returns a list of hosts. All query parameters are forwarded to Icinga2 and then filtered by hostgroup.
#[get("/icinga2/v1/objects/hosts?<params..>")]
pub async fn get_hosts(
    auth: AuthenticatedUser,
    config: &State<ProxyConfig>,
    params: HashMap<String, String>,
) -> Result<String, Status> {
    let text = get_icinga_hosts(config.inner(), Some(params)).await?;

    // Parse the JSON response from Icinga2
    let mut icinga_response: IcingaResponse = serde_json::from_str(&text)
        .map_err(|_| Status::InternalServerError)?;

    // Filter hosts based on user hostgroup
    icinga_response.results.retain(|host| {
        host.attrs.groups.contains(&auth.user_config.hostgroup)
    });

    // Convert back to JSON string
    let filtered_response = serde_json::to_string(&icinga_response).unwrap();

    Ok(filtered_response)
}
