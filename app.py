import threading
import cv2
import requests
from deepface import DeepFace
import time

ESP_IP = "192.168.4.1"
DOOR_ID = "door123"
LED_DURATION = 5  # seconds

cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

counter = 0
face_match = False
match_name = "Unknown"
reference = r"C:\Users\trist\OneDrive\Documenten\Bureaublad\face test\data"

def set_led(color):
    url = f"http://{ESP_IP}/esp32/led"
    payload = {"color": color}
    headers = {"Content-Type": "application/json"}
    try:
        response = requests.post(url, json=payload, headers=headers)
        if response.status_code == 200:
            print(f"LED set to {color}")
        else:
            print("Failed to set LED:", response.status_code, response.text)
    except Exception as e:
        print("Error connecting to ESP32:", e)

def check_face(frame):
    global face_match, match_name
    try:
        # Resize frame for faster processing
        small_frame = cv2.resize(frame, (160, 120))
        result = DeepFace.find(
            img_path=small_frame,
            db_path=reference,
            enforce_detection=False,
            detector_backend="opencv"
        )
        if len(result) > 0 and not result[0].empty:
            best_match = result[0].iloc[0]
            distance = best_match["distance"]
            identity = best_match["identity"]
            person_name = identity.split("\\")[-2]

            if distance < 0.5:
                if not face_match:
                    threading.Thread(target=unlock_led).start()
                face_match = True
                match_name = person_name + f" ({distance:.2f})"
            else:
                face_match = False
                match_name = "Unknown"
        else:
            face_match = False
            match_name = "Unknown"
    except Exception as e:
        face_match = False
        match_name = "Unknown"

def unlock_led():
    set_led("green")
    time.sleep(LED_DURATION)
    set_led("red")

while True:
    ret, frame = cap.read()
    if ret:
        # Process every 60 frames for speed
        if counter % 60 == 0:
            threading.Thread(target=check_face, args=(frame.copy(),)).start()
        counter += 1

        text = f"Matched: {match_name}" if face_match else "No Match"
        color = (0, 255, 0) if face_match else (0, 0, 255)
        cv2.putText(frame, text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, color, 2)
        cv2.imshow("Face Recognition", frame)

    if cv2.waitKey(1) == ord('q'):
        break

cv2.destroyAllWindows()
cap.release()
