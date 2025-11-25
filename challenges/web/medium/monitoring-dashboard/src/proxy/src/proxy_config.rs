use std::time::Duration;
use serde::{Deserialize, Serialize};
use rusqlite::{Connection, Result as SqliteResult, Row, OptionalExtension};
use uuid::Uuid;
use time::{OffsetDateTime, Duration as TimeDuration};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UserConfig {
    pub id: i64,
    pub username: String,
    pub password: String,
    pub hostgroup: String,
}

impl UserConfig {
    fn from_row(row: &Row) -> SqliteResult<Self> {
        Ok(Self {
            id: row.get(0)?,
            username: row.get(1)?,
            password: row.get(2)?,
            hostgroup: row.get(3)?,
        })
    }
}

pub struct Database {
    path: String,
}

impl Database {
    pub fn new() -> Self {
        Self {
            path: std::env::var("USERS_DB_PATH").unwrap_or_else(|_| "/app/users.db".to_string()),
        }
    }

    fn connection(&self) -> SqliteResult<Connection> {
        Connection::open(&self.path)
    }

    pub fn authenticate_user(&self, username: &str, password: &str) -> SqliteResult<Option<UserConfig>> {
        let conn = self.connection()?;

        conn.query_row(
            "SELECT id, username, password, hostgroup FROM users WHERE username = ? AND password = ?",
            [username, password],
            UserConfig::from_row
        ).optional()
    }

    pub fn create_session(&self, user_id: i64) -> SqliteResult<String> {
        let conn = self.connection()?;

        let session_id = Uuid::new_v4().to_string();
        let now = OffsetDateTime::now_utc().unix_timestamp();
        let expires_at = (OffsetDateTime::now_utc() + TimeDuration::hours(24)).unix_timestamp();

        conn.execute(
            "INSERT INTO sessions (session_id, user_id, created_at, expires_at) VALUES (?, ?, ?, ?)",
            [&session_id, &user_id.to_string(), &now.to_string(), &expires_at.to_string()],
        )?;

        Ok(session_id)
    }

    pub fn get_user_by_session(&self, session_id: &str) -> SqliteResult<Option<UserConfig>> {
        let conn = self.connection()?;

        // Clean expired sessions first
        let now = OffsetDateTime::now_utc().unix_timestamp();
        conn.execute("DELETE FROM sessions WHERE expires_at < ?", [now])?;

        conn.query_row(
            "SELECT u.id, u.username, u.password, u.hostgroup
             FROM users u
             JOIN sessions s ON u.id = s.user_id
             WHERE s.session_id = ? AND s.expires_at > ?",
            [session_id, &now.to_string()],
            UserConfig::from_row
        ).optional()
    }

    pub fn invalidate_session(&self, session_id: &str) -> SqliteResult<()> {
        let conn = self.connection()?;
        conn.execute("DELETE FROM sessions WHERE session_id = ?", [session_id])?;
        Ok(())
    }
}

pub struct ProxyConfig {
    pub icinga2_endpoint: String,
    pub icinga2_username: String,
    pub icinga2_password: String,
    pub external_connection_timeout: Duration,
    pub database: Database,
}

impl ProxyConfig {
    pub fn new_from_env() -> Self {
        Self {
            icinga2_endpoint: std::env::var("ICINGA2_ENDPOINT")
                .expect("ICINGA2_ENDPOINT environment variable must be set"),
            icinga2_username: std::env::var("ICINGA2_USERNAME")
                .expect("ICINGA2_USERNAME environment variable must be set"),
            icinga2_password: std::env::var("ICINGA2_PASSWORD")
                .expect("ICINGA2_PASSWORD environment variable must be set"),
            external_connection_timeout: Duration::from_secs(
                std::env::var("EXTERNAL_CONNECTION_TIMEOUT")
                    .expect("EXTERNAL_CONNECTION_TIMEOUT environment variable must be set")
                    .parse()
                    .expect("EXTERNAL_CONNECTION_TIMEOUT must be a valid integer"),
            ),
            database: Database::new(),
        }
    }

    // Convenience methods that delegate to the database
    pub fn authenticate_user(&self, username: &str, password: &str) -> SqliteResult<Option<UserConfig>> {
        self.database.authenticate_user(username, password)
    }

    pub fn create_session(&self, user_id: i64) -> SqliteResult<String> {
        self.database.create_session(user_id)
    }

    pub fn get_user_by_session(&self, session_id: &str) -> SqliteResult<Option<UserConfig>> {
        self.database.get_user_by_session(session_id)
    }

    pub fn invalidate_session(&self, session_id: &str) -> SqliteResult<()> {
        self.database.invalidate_session(session_id)
    }
}
