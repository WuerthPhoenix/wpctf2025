import datetime
import sqlite3
import jwt
from flask import Flask, request, render_template, redirect, make_response

app = Flask(__name__)

DB_PATH = "database.db"     # SQLite DB

def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def get_user_from_db(username):
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT * FROM users WHERE username = ?", (username,))
    user = cur.fetchone()
    conn.close()
    return user

def get_user():
    token = request.cookies.get("token")
    if not token:
        return None
    return jwt.decode(token, "supersecretkey", algorithms=["HS256"], options={"verify_signature": False})

def require_login(f):
    def wrapper(*args, **kwargs):
        user = get_user()
        if not user:
            return redirect("/")
        request.user = user
        return f(*args, **kwargs)
    wrapper.__name__ = f.__name__
    return wrapper

# ===== Routes =====
@app.route("/", methods=["GET", "POST"])
def login():
    if get_user():
        return redirect("/dashboard")

    if request.method == "POST":
        username = request.form.get("username")
        password = request.form.get("password")

        user = get_user_from_db(username)

        if not user or user["password"] != password:
            return render_template("index.html", message="Invalid credentials"), 401

        payload = {
            "username": user["username"],
            "role": user["role"],
            "exp": int((datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(hours=1)).timestamp()),
        }

        token = jwt.encode(payload, "supersecretkey", algorithm="HS256")
        resp = make_response(redirect("/dashboard"))
        resp.set_cookie("token", token, httponly=True)
        return resp

    return render_template('index.html')


@app.route("/dashboard")
@require_login
def dashboard():
    username = request.user.get("username")
    role = request.user.get("role")
    q = request.args.get("q", "").strip()
    page = int(request.args.get("page", 1))
    per_page = 10
    offset = (page - 1) * per_page

    try:
        sql = None
        count_sql = None
        results = []
        total_pages = 0

        if role == "agent_base":
            sql = f"SELECT * FROM communications WHERE role = 'agent_base' LIMIT {per_page} OFFSET {offset}"
            count_sql = "SELECT * FROM communications WHERE role = 'agent_base'"
        elif q and role == "superior_officer":
            sql = f"SELECT * FROM communications WHERE subject='{q}' LIMIT {per_page} OFFSET {offset}"
            count_sql = f"SELECT * FROM communications WHERE subject='{q}'"
        if sql:
            conn = get_db_connection()
            cursor = conn.cursor()

            cursor.execute(sql)
            results = cursor.fetchall()

            cursor.execute(count_sql)
            total_rows = len(cursor.fetchall())
            total_pages = (total_rows + per_page - 1) // per_page

            conn.close()
    except Exception as e:
        return f"SQL error: {str(e)}", 500

    if request.headers.get("X-Requested-With") == "XMLHttpRequest":
        return render_template("partials/results_table.html", results=results, q=q, page=page, total_pages=total_pages)

    return render_template("dashboard.html", username=username, role=role, q=q, results=results, page=page, total_pages=total_pages)


@app.route("/logout")
def logout():
    resp = make_response(redirect("/"))
    resp.set_cookie("token", "", expires=0)
    return resp

if __name__ == "__main__":
    app.run(host="0.0.0.0",port=4759)
