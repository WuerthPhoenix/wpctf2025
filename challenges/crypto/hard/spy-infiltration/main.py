from os import environ
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
from base64 import b64encode, b64decode
from flask import Flask, render_template, request, jsonify, redirect

app = Flask(__name__, static_folder="static", template_folder="templates")

SECRET = environ.get("SECRET").encode("utf-8")
FLAG = environ.get("FLAG")


def netstring(s: str) -> str:
    return f"{len(s)}:{s},"


def read_netstring(ns: str) -> tuple[str, str]:
    length, rest = ns.split(":", 1)
    length = int(length)
    s, comma = rest[:length], rest[length : length + 1]
    if comma != ",":
        raise ValueError("Invalid netstring")
    return s, rest[length + 1 :]


def encrypt(msg: str) -> str:
    cipher = AES.new(SECRET, AES.MODE_ECB)
    ct_bytes = cipher.encrypt(pad(msg.encode(), AES.block_size))
    return b64encode(ct_bytes).decode()


def decrypt(ct: str) -> str:
    cipher = AES.new(SECRET, AES.MODE_ECB)
    pt = unpad(cipher.decrypt(b64decode(ct)), AES.block_size)
    return pt.decode()


def save_config(comment: str, baud_rate: str, timezone: str, mode: str) -> None:
    config = (
        netstring("version: 1.0")
        + netstring(f"comment: {comment}")
        + netstring(f"password: {FLAG}")
        + netstring(f"baudrate: {baud_rate}")
        + netstring(f"timezone: {timezone}")
        + netstring(f"mode: {mode}")
    )
    with open("config.ns.aes", "w") as f:
        print(config)
        _ = f.write(encrypt(config))


def read_config() -> dict[str, str]:
    with open("config.ns.aes", "r") as f:
        encrypted_config = f.read().strip()

    decrypted = decrypt(encrypted_config)
    config: dict[str, str] = {}
    while decrypted:
        key_value, decrypted = read_netstring(decrypted)
        key, value = key_value.split(":", 1)
        if key.strip() != "password":  # Do not include password in the returned config
            config[key.strip()] = value.strip()
    return config


@app.route("/")
def redirect_to_config():
    return redirect("/config")


@app.route("/config")
def serve_app():
    """Serves the main HTML file."""
    return render_template("index.html", config=read_config())


@app.route("/config", methods=["POST"])
def save_config_endpoint():
    global saved_config

    if not request.is_json:
        return jsonify({"error": "Request must be JSON"}), 400

    data = request.get_json()
    if not data:
        return jsonify({"error": "JSON payload required"}), 400

    try:
        save_config(
            comment=data.get("comment"),
            baud_rate=data.get("baud_rate"),
            timezone=data.get("timezone"),
            mode=data.get("mode"),
        )
        return jsonify({"message": "successfully saved!"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route("/config/download", methods=["GET"])
def download_config():
    with open("config.ns.aes", "r") as f:
        encrypted_config = f.read().strip()
    return jsonify({"encrypted_config": encrypted_config}), 200


if __name__ == "__main__":
    save_config("Device active in Sector 7G.", "57600", "UTC+1", "default")
    app.run(host="0.0.0.0", port=5000)
