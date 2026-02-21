import time
import json
import paho.mqtt.client as mqtt
from kafka import KafkaConsumer
from kafka.errors import KafkaError
from collections import deque
from threading import Lock
from datetime import datetime
import pytz

MQTT_BROKER = "broker.hivemq.com"   # or: test.mosquitto.org, broker.emqx.io
MQTT_PORT = 1883
MQTT_BASE_TOPIC = "demo/people-counting"   # camera ID appended: demo/people-counting/<camera_id>
MQTT_USE_AUTH = False
MQTT_USERNAME = ""      # set if MQTT_USE_AUTH is True
MQTT_PASSWORD = ""      # set if MQTT_USE_AUTH is True

# -----------------------------------------------------------------------------
# Kafka
# -----------------------------------------------------------------------------
KAFKA_TOPIC = "quickstart-events"
KAFKA_BROKER = "localhost:9092"

# -----------------------------------------------------------------------------
# Source/Camera IDs — adjust to your areas and cameras
# -----------------------------------------------------------------------------
COMPANY_ID = "company-001"
SOURCE_CONFIG = {
    0: {"AreaID": "area-001", "CameraID": "camera-001"},
    1: {"AreaID": "area-001", "CameraID": "camera-002"},
}

# Previous counts storage
previous_counts = {}
source_occupancy = {}  # New dictionary to track occupancy per source

# Buffer for messages
message_buffer = deque(maxlen=1000)  # Limit buffer size to prevent memory issues
buffer_lock = Lock()  # Thread-safe operations on buffer

def connect_mqtt():
    mqtt_client = mqtt.Client()
    if MQTT_USE_AUTH and MQTT_USERNAME:
        mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        print(f"Connected to MQTT broker {MQTT_BROKER}:{MQTT_PORT}")
        return mqtt_client
    except Exception as e:
        print(f"Failed to connect to MQTT broker: {e}")
        return None

def format_message(message):
    try:
        # Get source_id from the message
        source_id = int(message.get('analyticsModule', {}).get('source_id', 0))
        
        # Get current timestamp in Asia/Singapore timezone (+08:00)
        tz = pytz.timezone('Asia/Singapore')
        current_time = datetime.now(tz)
        
        # Get current counts
        current_entry = message.get('analyticsModule', {}).get('Entry', 0)
        current_exit = message.get('analyticsModule', {}).get('Exit', 0)
        
        # Initialize source tracking if not exists
        if source_id not in previous_counts:
            previous_counts[source_id] = {'Entry': 0, 'Exit': 0}
            source_occupancy[source_id] = 0
        
        # Calculate deltas
        entry_delta = max(0, current_entry - previous_counts[source_id]['Entry'])
        exit_delta = max(0, current_exit - previous_counts[source_id]['Exit'])
        
        # Update source-specific occupancy
        source_occupancy[source_id] = max(0, source_occupancy[source_id] + entry_delta - exit_delta)
        
        # Update previous counts for next comparison
        previous_counts[source_id]['Entry'] = current_entry
        previous_counts[source_id]['Exit'] = current_exit
        
        # Only create message if there are changes
        if entry_delta > 0 or exit_delta > 0:
            cfg = SOURCE_CONFIG.get(source_id, {"AreaID": f"area-{source_id}", "CameraID": f"camera-{source_id}"})
            formatted_message = {
                "source_id": "deepstream-occupancy",
                "company_id": COMPANY_ID,
                "area_id": cfg["AreaID"],
                "camera_id": cfg["CameraID"],
                "timestamp": current_time.strftime('%Y-%m-%d %H:%M:%S'),
                "timezone": "+08:00",
                "Entry": entry_delta,  # Send the pulse
                "Exit": exit_delta,    # Send the pulse
                "occupancy": source_occupancy[source_id]  # Use source-specific occupancy
            }
            return formatted_message
        return None
        
    except Exception as e:
        print(f"Error formatting message: {e}")
        return None

def process_and_send_buffer(mqtt_client):
    global message_buffer
    
    with buffer_lock:
        if len(message_buffer) > 0:
            # Get the most recent message
            latest_message = message_buffer[-1]
            message_buffer.clear()  # Clear buffer after getting latest message
            
            try:
                # Format message according to required structure
                formatted_message = format_message(latest_message)
                if formatted_message:  # Only publish if there are changes
                    # Construct topic with camera ID
                    camera_specific_topic = f"{MQTT_BASE_TOPIC}/{formatted_message['camera_id']}"
                    
                    # Publish formatted message to MQTT
                    mqtt_client.publish(camera_specific_topic, json.dumps(formatted_message))
                    print(f"Published pulse to MQTT topic {camera_specific_topic}: {formatted_message}")
                    
                    # Immediately send a follow-up message with Entry/Exit reset to 0
                    formatted_message["Entry"] = 0
                    formatted_message["Exit"] = 0
                    mqtt_client.publish(camera_specific_topic, json.dumps(formatted_message))
                    print(f"Published reset to MQTT topic {camera_specific_topic} (Entry=0, Exit=0)")
                    
            except Exception as e:
                print(f"Error publishing to MQTT: {e}")

def main():
    # Initialize MQTT Client
    mqtt_client = connect_mqtt()
    if not mqtt_client:
        return

    try:
        # Kafka Consumer
        consumer = KafkaConsumer(
            KAFKA_TOPIC,
            bootstrap_servers=KAFKA_BROKER,
            auto_offset_reset='latest',  # Only get new messages
            enable_auto_commit=True,
            group_id='occupancy-detection-consumer',  # Fixed group ID
            value_deserializer=lambda x: json.loads(x.decode("utf-8")),
        )

        print("Connected to Kafka broker. Waiting for messages...")
        
        last_send_time = time.time()
        
        while True:
            try:
                # Poll for messages with a timeout
                message_batch = consumer.poll(timeout_ms=100)
                
                current_time = time.time()
                
                # Process messages into buffer
                if message_batch:
                    for tp, messages in message_batch.items():
                        for message in messages:
                            try:
                                with buffer_lock:
                                    message_buffer.append(message.value)
                                print(f"Buffered message: {message.value}")
                            except Exception as e:
                                print(f"Error processing message: {e}")
                                continue
                
                # Check if it's time to send (every 1 second)
                if current_time - last_send_time >= 1.0:
                    process_and_send_buffer(mqtt_client)
                    last_send_time = current_time
                
                # Small sleep to prevent CPU spinning
                time.sleep(0.01)

            except Exception as e:
                print(f"Error polling messages: {e}")
                time.sleep(1)  # Wait before retrying
                continue

    except KafkaError as e:
        print(f"Kafka error: {e}")
    except KeyboardInterrupt:
        print("Shutting down...")
    finally:
        # Cleanup
        try:
            consumer.close()
            mqtt_client.disconnect()
            print("Connections closed properly")
        except:
            pass

if __name__ == "__main__":
    main()
