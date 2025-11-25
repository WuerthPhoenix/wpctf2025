#!/usr/bin/env python3

import paho.mqtt.client as mqtt
import time
import os
import json
import base64
import random

# MQTT Configuration
MQTT_BROKER = os.getenv('MQTT_BROKER', 'localhost')
MQTT_PORT = int(os.getenv('MQTT_PORT', '1883'))

# Hidden topic - only accessible with leaked credentials
HIDDEN_TOPIC = "internal/admin/secure_channel"
CREDENTIAL_TOPIC = "credentials/unlock"

# Leaked user credentials (base64 encoded)
LEAKED_USERNAME = os.getenv('ADMIN_USERNAME')
LEAKED_PASSWORD = os.getenv('ADMIN_PASSWORD')
LEAKED_CREDENTIALS_B64 = base64.b64encode(f"{LEAKED_USERNAME}:{LEAKED_PASSWORD}".encode()).decode()

# Challenge credentials
CREDENTIALS = {
    "username": os.getenv('CHARGER_USERNAME'),
    "password": os.getenv('CHARGER_PASSWORD'),
}

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Credential server connected to MQTT broker")
        client.subscribe(HIDDEN_TOPIC)
        print(f"Subscribed to hidden topic: {HIDDEN_TOPIC}")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    """Handle messages on the hidden topic"""
    try:
        topic = msg.topic
        payload = msg.payload.decode('utf-8')
        
        print(f"Received message on {topic}: {payload}")
        
        # Check if the message is the correct challenge response
        if topic == HIDDEN_TOPIC:
            # Simple challenge - user needs to send specific message
            if payload.strip().lower() == "Trust me I am the admin".lower():
                print("Correct challenge response received! Sending credentials...")
                
                # Send credentials to credential topic
                credential_message = json.dumps(CREDENTIALS, indent=2)
                client.publish(CREDENTIAL_TOPIC, credential_message)
                print(f"Credentials sent to {CREDENTIAL_TOPIC}")
                
                # Also send a confirmation message
                confirmation = {
                    "status": "success",
                    "message": "Credentials unlocked! Check topic: " + CREDENTIAL_TOPIC,
                    "timestamp": int(time.time())
                }
                client.publish(HIDDEN_TOPIC + "/response", json.dumps(confirmation))
                
            else:
                # Send hint for incorrect attempts
                hint = {
                    "status": "error",
                    "message": "Incorrect challenge response. Try again.",
                    "hint": "Send the right message to unlock the credentials..."
                }
                client.publish(HIDDEN_TOPIC + "/response", json.dumps(hint))
                
    except Exception as e:
        print(f"Error handling message: {e}")

def publish_spam_backup():
    """Periodically leak credentials within normal-looking messages"""
    print("Starting credential leak thread...")
    
    # Retry connection
    max_retries = 30
    retry_count = 0
    client = None
    
    while retry_count < max_retries and client is None:
        try:
            client = mqtt.Client()
            client.connect(MQTT_BROKER, MQTT_PORT, 60)
            client.loop_start()
            print("Credential leak client connected and started")
            break
        except Exception as e:
            print(f"Credential leak client connection failed: {e}")
            retry_count += 1
            time.sleep(2)
    
    if client is None:
        print("Credential leak client failed to connect after retries")
        return
    
    while True:
        delay = random.randint(10, 30)
        print(f"Credential leak sleeping for {delay} seconds...")
        time.sleep(delay)
        
        # Generate dynamic realistic data
        current_time = time.time()
        session_id = ''.join(random.choices('0123456789abcdef', k=16))
        host_num = random.randint(1, 5)
        charger_id = f"CHG-{random.randint(1, 999):03d}-{random.choice(['A', 'B', 'C'])}"
        thread_num = random.randint(1, 8)
        
        leak_messages = [
            {
                "topic": "logs/system",
                "message": {
                    "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(current_time)) + f".{random.randint(100,999)}Z",
                    "level": random.choice(["DEBUG", "INFO", "WARN"]),
                    "service": "authentication-service",
                    "host": f"auth-{host_num:02d}.internal",
                    "message": random.choice([
                        "OAuth2 token validation successful",
                        "JWT token refresh completed",
                        "Session authentication verified",
                        "API key validation passed"
                    ]),
                    "session_id": f"sess_{session_id}",
                    "client_ip": f"10.0.{random.randint(1,255)}.{random.randint(1,255)}",
                    "user_agent": f"ChargingStation/{random.randint(2,3)}.{random.randint(0,9)}.{random.randint(0,9)} (Linux; arm64)",
                    "endpoint": random.choice([
                        "/api/v1/auth/validate",
                        "/api/v1/auth/refresh",
                        "/api/v1/session/verify",
                        "/api/v1/tokens/check"
                    ])
                }
            },
            {
                "topic": "monitoring/health",
                "message": {
                    "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(current_time)) + f".{random.randint(100,999)}Z",
                    "service": random.choice(["postgresql-primary", "postgresql-replica", "redis-cluster"]),
                    "status": random.choice(["healthy", "degraded", "recovering"]),
                    "host": f"db-{host_num:02d}.internal",
                    "metrics": {
                        "connections": random.randint(5, 50),
                        "cpu_usage": round(random.uniform(10, 80), 1),
                        "memory_usage": round(random.uniform(0.5, 4.0), 1)
                    },
                    "connection_pool": f"postgresql://dbservice:{base64.b64encode(f'dbservice:database_passw0rd'.encode()).decode()}@db-{host_num:02d}.internal:5432/charging_db",
                    "last_backup": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(current_time - random.randint(3600, 28800))),
                    "replica_lag": f"{random.randint(0, 100)}ms"
                }
            },
            {
                "topic": "logs/application",
                "message": {
                    "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(current_time)) + f".{random.randint(100,999)}Z",
                    "level": random.choice(["INFO", "DEBUG", "NOTICE"]),
                    "service": "charging-controller",
                    "host": f"controller-{host_num:02d}.internal",
                    "thread": f"worker-thread-{thread_num}",
                    "message": random.choice([
                        "Charger initialization sequence completed",
                        "Power management system synchronized",
                        "Vehicle connection established",
                        "Charging session initiated"
                    ]),
                    "charger_id": charger_id,
                    "initialization_data": {
                        "firmware_version": f"v{random.randint(2,4)}.{random.randint(0,9)}.{random.randint(0,9)}",
                        "max_power": f"{random.choice([22, 50, 150, 350])}kW",
                        "connectors": random.sample(["CCS", "CHAdeMO", "Type2", "Tesla"], k=random.randint(1, 3)),
                    },
                    "status": random.choice(["online", "charging", "available", "maintenance"]),
                    "grid_connection": random.choice(["stable", "fluctuating", "peak_load"])
                }
            },
            {
                "topic": "monitoring/performance",
                "message": {
                    "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(current_time)) + f".{random.randint(100,999)}Z",
                    "service": "load-balancer",
                    "host": f"lb-{host_num:02d}.internal",
                    "metrics": {
                        "requests_per_second": round(random.uniform(50, 500), 1),
                        "avg_response_time": round(random.uniform(20, 200), 1),
                        "error_rate": round(random.uniform(0.01, 0.1), 3),
                        "active_connections": random.randint(500, 2000)
                    },
                    "backend_health": {
                        f"api-{i:02d}": random.choice(["healthy", "degraded", "unhealthy"]) 
                        for i in range(1, random.randint(3, 6))
                    },
                    "cert_expires": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(current_time + random.randint(86400*30, 86400*365)))
                }
            }
        ]
        
        # Randomly pick one leak message
        leak = random.choice(leak_messages)
        result = client.publish(leak["topic"], json.dumps(leak["message"]))
        print(f"Published credential leak to {leak['topic']}: {result}")
        
        # Random delay before next leak
        time.sleep(random.randint(30, 60))

def publish_session_token():
    """Publish session_token every 60 seconds predictably"""
    print("Starting session_token publisher thread...")
    
    # Retry connection
    max_retries = 30
    retry_count = 0
    client = None
    
    while retry_count < max_retries and client is None:
        try:
            client = mqtt.Client()
            client.connect(MQTT_BROKER, MQTT_PORT, 60)
            client.loop_start()
            print("Session_token publisher client connected")
            break
        except Exception as e:
            print(f"Session_token publisher connection failed: {e}")
            retry_count += 1
            time.sleep(2)
    
    if client is None:
        print("Session_token publisher failed to connect after retries")
        return
    
    while True:
        time.sleep(60)  # Wait 60 seconds
        
        # Generate current time data
        current_time = time.time()
        session_id = ''.join(random.choices('0123456789abcdef', k=16))
        
        # Create session_token message
        session_token_message = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(current_time)) + f".{random.randint(100,999)}Z",
            "event_type": random.choice(["authentication_success", "authorization_granted", "token_issued"]),
            "severity": random.choice(["info", "notice", "warning"]),
            "source": random.choice(["api-gateway", "auth-proxy", "load-balancer"]),
            "user_identity": {
                "username": random.choice(["service_account", "admin_service", "system_user"]),
                "role": random.choice(["admin", "operator", "service"]),
                "permissions": random.sample(["read", "write", "admin", "monitor", "control"], k=random.randint(2, 4))
            },
            "request_details": {
                "method": random.choice(["POST", "GET", "PUT", "PATCH"]),
                "endpoint": random.choice([
                    "/api/v1/chargers/status",
                    "/api/v1/users/profile",
                    "/api/v1/stations/control",
                    "/api/v1/billing/transactions"
                ]),
                "source_ip": f"192.168.{random.randint(1,255)}.{random.randint(1,255)}",
                "x_forwarded_for": f"203.0.{random.randint(1,255)}.{random.randint(1,255)}"
            },
            "session_info": {
                "session_id": f"sess_admin_{session_id[:8]}",
                "session_token": LEAKED_CREDENTIALS_B64,
                "expires_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(current_time + random.randint(1800, 7200)))
            }
        }
        
        result = client.publish("security/events", json.dumps(session_token_message))
        print(f"Published session_token to security/events: {result}")

def publish_admin_hints():
    """Publish hints for admin users on how to access the secure channel"""
    print("Starting admin hints thread...")
    
    # Retry connection
    max_retries = 30
    retry_count = 0
    client = None
    
    while retry_count < max_retries and client is None:
        try:
            client = mqtt.Client()
            client.username_pw_set(LEAKED_USERNAME, LEAKED_PASSWORD)
            client.connect(MQTT_BROKER, MQTT_PORT, 60)
            client.loop_start()
            print("Admin hints client connected")
            break
        except Exception as e:
            print(f"Admin hints client connection failed: {e}")
            retry_count += 1
            time.sleep(2)
    
    if client is None:
        print("Admin hints client failed to connect after retries")
        return
    
    while True:
        time.sleep(5)
        
        # Publish hints that only admin users can see
        hint_messages = [
            {
                "topic": "admin/notices",
                "message": {
                    "notice": "Secure administrative channel available",
                    "location": "internal/admin/secure_channel",
                    "timestamp": int(time.time())
                }
            },
            {
                "topic": "system/maintenance",
                "message": {
                    "service": "credential_manager",
                    "status": "awaiting unlock command",
                    "encrypted_unlock_phrase": os.getenv('ENCRYPTED_UNLOCK_PHRASE'),
                    "timestamp": int(time.time())
                }
            }
        ]
        
        hint = random.choice(hint_messages)
        result = client.publish(hint["topic"], json.dumps(hint["message"]))
        print(f"Published admin hint to {hint['topic']}: {result}")

def main():
    max_retries = 30
    retry_count = 0
    
    while retry_count < max_retries:
        try:
            # Main client connects as admin_user to publish to ctf/ topics
            client = mqtt.Client()
            client.username_pw_set(LEAKED_USERNAME, LEAKED_PASSWORD)
            client.on_connect = on_connect
            client.on_message = on_message
            
            print(f"Starting credential server, connecting to {MQTT_BROKER}:{MQTT_PORT} (attempt {retry_count + 1})")
            client.connect(MQTT_BROKER, MQTT_PORT, 60)
            
            # Start credential leak publishing in background
            import threading
            print("Starting credential leak thread...")
            leak_thread = threading.Thread(target=publish_spam_backup)
            leak_thread.daemon = True
            leak_thread.start()
            print("Credential leak thread started")
            
            # Start session_token publishing in background
            print("Starting session_token thread...")
            token_thread = threading.Thread(target=publish_session_token)
            token_thread.daemon = True
            token_thread.start()
            print("Session_token thread started")
            
            # Start admin hints publishing in background
            print("Starting admin hints thread...")
            hint_thread = threading.Thread(target=publish_admin_hints)
            hint_thread.daemon = True
            hint_thread.start()
            print("Admin hints thread started")
            
            # Keep the client running
            client.loop_forever()
            break
            
        except Exception as e:
            print(f"Error connecting to MQTT broker: {e}")
            retry_count += 1
            if retry_count >= max_retries:
                print("Max retries reached, exiting")
                return
            time.sleep(2)  # Wait 2 seconds before retry

if __name__ == "__main__":
    main()