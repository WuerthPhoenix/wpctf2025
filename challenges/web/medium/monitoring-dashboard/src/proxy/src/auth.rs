use rocket::serde::{Deserialize, Serialize, json::Json};
use rocket::{post, delete, get, State};
use rocket::http::{Cookie, CookieJar, Status, SameSite};
use time::Duration as TimeDuration;
use crate::proxy_config::ProxyConfig;
use crate::session_auth::AuthenticatedUser;

#[derive(Deserialize)]
#[serde(crate = "rocket::serde")]
pub struct LoginRequest {
    pub username: String,
    pub password: String,
}

#[derive(Serialize)]
#[serde(crate = "rocket::serde")]
pub struct LoginResponse {
    pub success: bool,
    pub message: String,
    pub username: Option<String>,
}

#[derive(Serialize)]
#[serde(crate = "rocket::serde")]
pub struct LogoutResponse {
    pub success: bool,
    pub message: String,
}

#[derive(Serialize)]
#[serde(crate = "rocket::serde")]
pub struct MeResponse {
    pub success: bool,
    pub username: Option<String>,
}

/// Login endpoint - authenticates user and sets session cookie
#[post("/login", data = "<login_request>")]
pub async fn login(
    login_request: Json<LoginRequest>,
    config: &State<ProxyConfig>,
    cookies: &CookieJar<'_>,
) -> Result<Json<LoginResponse>, Status> {
    match config.authenticate_user(&login_request.username, &login_request.password) {
        Ok(Some(user_config)) => {
            // Create session
            match config.create_session(user_config.id) {
                Ok(session_id) => {
                    // Set secure session cookie
                    let mut cookie = Cookie::new("session_id", session_id);
                    cookie.set_http_only(true);
                    cookie.set_same_site(SameSite::Lax);
                    cookie.set_max_age(TimeDuration::hours(24));
                    cookie.set_path("/");

                    cookies.add_private(cookie);

                    Ok(Json(LoginResponse {
                        success: true,
                        message: "Login successful".to_string(),
                        username: Some(user_config.username)
                    }))
                }
                Err(_) => Err(Status::InternalServerError),
            }
        }
        Ok(None) => Ok(Json(LoginResponse {
            success: false,
            message: "Invalid credentials".to_string(),
            username: None,
        })),
        Err(_) => Err(Status::InternalServerError),
    }
}

/// Logout endpoint - invalidates session and removes cookie
#[delete("/logout")]
pub async fn logout(
    config: &State<ProxyConfig>,
    cookies: &CookieJar<'_>,
) -> Json<LogoutResponse> {
    if let Some(session_cookie) = cookies.get_private("session_id") {
        // Invalidate session in database
        let _ = config.invalidate_session(session_cookie.value());

        // Remove cookie
        cookies.remove_private(Cookie::build("session_id"));

        Json(LogoutResponse {
            success: true,
            message: "Logout successful".to_string(),
        })
    } else {
        Json(LogoutResponse {
            success: false,
            message: "No active session".to_string(),
        })
    }
}

/// Me endpoint - returns current user information
#[get("/me")]
pub async fn me(
    user: AuthenticatedUser,
) -> Json<MeResponse> {
    Json(MeResponse {
        success: true,
        username: Some(user.user_config.username),
    })
}
