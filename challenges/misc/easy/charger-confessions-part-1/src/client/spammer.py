#!/usr/bin/env python3

import paho.mqtt.client as mqtt
import time
import random
import os
import json
import threading
import base64

# MQTT Configuration
MQTT_BROKER = os.getenv('MQTT_BROKER', 'localhost')
MQTT_PORT = int(os.getenv('MQTT_PORT', '1883'))

# Topics to spam - including decoy topics that look like the real one
SPAM_TOPICS = [
    'sensors/temperature',
    'sensors/humidity',
    'sensors/pressure',
    'devices/status',
    'logs/application',
    'logs/system',
    'alerts/warnings',
    'alerts/errors',
    'data/metrics',
    'data/analytics',
    'network/traffic',
    'network/bandwidth',
    'security/access',
    'security/events',
    'monitoring/health',
    'monitoring/performance',
    'public/announcements',
    'status/general',
    'activity/logs',
    'challenge/info',
    'hidden/decoy1',
    'hidden/fake_secret',
    'secret/disabled',
    'admin/readonly',
    'system/auth/status',
    'system/maintenance/log'
]

def generate_realistic_data(topic):
    """Generate realistic data based on topic"""
    current_time = time.time()
    timestamp = time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(current_time)) + f".{random.randint(100,999)}Z"
    
    if topic.startswith('sensors/'):
        sensor_type = topic.split('/')[1]
        if sensor_type == 'temperature':
            return {
                'timestamp': timestamp,
                'sensor_id': f"TEMP_{random.randint(1,50):03d}",
                'value': round(random.uniform(18.0, 45.0), 1),
                'unit': 'celsius',
                'location': random.choice(['charging_bay_1', 'charging_bay_2', 'transformer_room', 'control_cabinet']),
                'status': random.choice(['normal', 'warning', 'critical'])
            }
        elif sensor_type == 'humidity':
            return {
                'timestamp': timestamp,
                'sensor_id': f"HUM_{random.randint(1,20):03d}",
                'value': round(random.uniform(30.0, 85.0), 1),
                'unit': 'percent',
                'location': random.choice(['outdoor_station', 'indoor_lobby', 'equipment_room']),
                'status': 'normal'
            }
        elif sensor_type == 'pressure':
            return {
                'timestamp': timestamp,
                'sensor_id': f"PRES_{random.randint(1,10):03d}",
                'value': round(random.uniform(980.0, 1020.0), 1),
                'unit': 'hPa',
                'location': 'weather_station',
                'trend': random.choice(['rising', 'falling', 'stable'])
            }
    
    elif topic.startswith('logs/'):
        log_type = topic.split('/')[1]
        if log_type == 'application':
            return {
                'timestamp': timestamp,
                'level': random.choice(['INFO', 'DEBUG', 'WARN', 'ERROR']),
                'service': random.choice(['charging-service', 'billing-service', 'user-service']),
                'message': random.choice([
                    'Vehicle connected to charging port',
                    'Charging session completed successfully',
                    'Payment processing initiated',
                    'User authentication successful',
                    'Charging rate adjusted automatically',
                    'System health check completed'
                ]),
                'session_id': f"sess_{random.randint(100000,999999)}",
                'user_id': f"user_{random.randint(1000,9999)}"
            }
        elif log_type == 'system':
            return {
                'timestamp': timestamp,
                'level': random.choice(['INFO', 'WARN', 'ERROR']),
                'component': random.choice(['power_management', 'network_controller', 'database']),
                'message': random.choice([
                    'System startup completed',
                    'Configuration reloaded',
                    'Backup process started',
                    'Network connection established',
                    'Database connection pool refreshed'
                ]),
                'pid': random.randint(1000, 9999),
                'host': f"server-{random.randint(1,5):02d}"
            }
    
    elif topic.startswith('network/'):
        net_type = topic.split('/')[1]
        if net_type == 'traffic':
            return {
                'timestamp': timestamp,
                'interface': random.choice(['eth0', 'eth1', 'wlan0']),
                'bytes_in': random.randint(1000000, 50000000),
                'bytes_out': random.randint(500000, 20000000),
                'packets_in': random.randint(1000, 50000),
                'packets_out': random.randint(500, 20000),
                'errors': random.randint(0, 5)
            }
        elif net_type == 'bandwidth':
            return {
                'timestamp': timestamp,
                'interface': random.choice(['eth0', 'eth1']),
                'utilization': round(random.uniform(10.0, 90.0), 1),
                'speed_mbps': random.choice([100, 1000, 10000]),
                'status': random.choice(['active', 'congested', 'normal'])
            }
    
    elif topic.startswith('alerts/'):
        alert_type = topic.split('/')[1]
        if alert_type == 'warnings':
            return {
                'timestamp': timestamp,
                'severity': 'warning',
                'component': random.choice(['charger_01', 'charger_02', 'transformer', 'cooling_system']),
                'message': random.choice([
                    'Temperature approaching threshold',
                    'Power consumption above normal',
                    'Cooling system efficiency reduced',
                    'Cable wear detected'
                ]),
                'alert_id': f"WARN_{random.randint(1000,9999)}"
            }
        elif alert_type == 'errors':
            return {
                'timestamp': timestamp,
                'severity': 'error',
                'component': random.choice(['payment_gateway', 'vehicle_interface', 'safety_system']),
                'message': random.choice([
                    'Payment processing failed',
                    'Vehicle communication timeout',
                    'Safety interlock activated',
                    'Ground fault detected'
                ]),
                'error_code': f"ERR_{random.randint(100,999)}"
            }
    
    elif topic.startswith('monitoring/'):
        mon_type = topic.split('/')[1]
        if mon_type == 'health':
            fake_creds = base64.b64encode(f"monitor_user:m0n1t0r_p@ss".encode()).decode()
            return {
                'timestamp': timestamp,
                'service': random.choice(['charging-controller', 'user-interface', 'payment-processor']),
                'status': random.choice(['healthy', 'degraded', 'unhealthy']),
                'response_time': round(random.uniform(50, 500), 1),
                'cpu_usage': round(random.uniform(20, 80), 1),
                'memory_usage': round(random.uniform(1.0, 8.0), 1),
                'uptime': random.randint(3600, 86400),
                'health_check_token': fake_creds
            }
        elif mon_type == 'performance':
            return {
                'timestamp': timestamp,
                'metric': random.choice(['charging_rate', 'user_sessions', 'api_calls']),
                'value': round(random.uniform(10, 1000), 1),
                'unit': random.choice(['kW', 'sessions', 'calls/min']),
                'trend': random.choice(['increasing', 'decreasing', 'stable'])
            }
    
    # Default fallback for any topic
    return {
        'timestamp': timestamp,
        'data': random.choice(['status_ok', 'processing', 'idle', 'active']),
        'value': random.randint(1, 100)
    }

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker successfully")
    else:
        print(f"Failed to connect, return code {rc}")

def spam_messages(client):
    """Continuously spam messages to random topics"""
    while True:
        topic = random.choice(SPAM_TOPICS)
        message_data = generate_realistic_data(topic)
        message = json.dumps(message_data)
        
        try:
            client.publish(topic, message)
            print(f"Published to {topic}: {message[:50]}...")
        except Exception as e:
            print(f"Error publishing to {topic}: {e}")
        
        # Random delay between messages
        time.sleep(random.uniform(0.1, 2.0))

def main():
    # Wait for MQTT broker to be ready
    max_retries = 30
    retry_count = 0
    
    while retry_count < max_retries:
        try:
            client = mqtt.Client()
            client.on_connect = on_connect
            
            print(f"Connecting to MQTT broker at {MQTT_BROKER}:{MQTT_PORT} (attempt {retry_count + 1})")
            client.connect(MQTT_BROKER, MQTT_PORT, 60)
            
            # Start the spam thread
            spam_thread = threading.Thread(target=spam_messages, args=(client,))
            spam_thread.daemon = True
            spam_thread.start()
            
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