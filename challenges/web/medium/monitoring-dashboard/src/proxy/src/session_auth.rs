use crate::proxy_config::{ProxyConfig, UserConfig};
use rocket::http::Status;
use rocket::request::{FromRequest, Outcome};
use rocket::{Request, State};
use rocket::http::Cookie;

#[derive(Debug)]
pub struct AuthenticatedUser {
    pub user_config: UserConfig,
}

#[derive(Debug, PartialEq, Eq)]
pub enum AuthError {
    InternalServerError,
    MissingSession,
    InvalidSession,
    DatabaseError,
}

#[rocket::async_trait]
impl<'r> FromRequest<'r> for AuthenticatedUser {
    type Error = AuthError;

    async fn from_request(request: &'r Request<'_>) -> Outcome<Self, Self::Error> {
        let Outcome::Success(config) = request.guard::<&State<ProxyConfig>>().await else {
            return Outcome::Error((Status::InternalServerError, AuthError::InternalServerError));
        };

        let cookies = request.cookies();

        // Check for session cookie
        if let Some(session_cookie) = cookies.get_private("session_id") {
            match config.get_user_by_session(session_cookie.value()) {
                Ok(Some(user_config)) => return Outcome::Success(AuthenticatedUser { user_config }),
                Ok(None) => {
                    // Session expired or invalid, remove cookie
                    cookies.remove_private(Cookie::build("session_id"));
                    return Outcome::Error((Status::Unauthorized, AuthError::InvalidSession));
                }
                Err(_) => return Outcome::Error((Status::InternalServerError, AuthError::DatabaseError)),
            }
        }

        // No valid session found
        Outcome::Error((Status::Unauthorized, AuthError::MissingSession))
    }
}
