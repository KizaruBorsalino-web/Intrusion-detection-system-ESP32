// ids_esp32_final.ino
// ESP32 IDS demo: runs TFLite model on UDP "features"
// Turns LED ON if suspicious packets are detected

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduTFLite.h>
#include "model.h" 

// ================== CONFIG ==================
constexpr int kTensorArenaSize = 50 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];

const int LED_PIN = 2;                // built-in LED pin (GPIO2 on most boards)
const float PRED_THRESHOLD = 0.5;     // probability threshold
const int FEATURE_COUNT = 122;        // must match your model input

// WiFi setup (ESP32 as AP for easy demo)
const char *ssid = "ESP32_IDS";
const char *password = "12345678";
WiFiUDP udp;
const int UDP_PORT = 3333;

// Normalization arrays (replace with your scaler from Python)
float means[FEATURE_COUNT] = {0};
float scales[FEATURE_COUNT] = {1};

// ========== TFLite objects ==========
tflite::AllOpsResolver resolver;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

// ========== LED Detection Control ==========
const int DETECT_REQUIRED = 2;        // require N consecutive detections
const unsigned long HOLD_MS = 5000UL; // keep LED on at least this long

int consecutive_detections = 0;
unsigned long last_detection_time = 0;
bool led_state = false;

// ========== Helpers ==========
float normalize_val(float x, float mean, float scale) {
  if (scale == 0.0f) return 0.0f;
  return (x - mean) / scale;
}

inline int8_t float_to_int8(float value, TfLiteTensor *tensor) {
  float scale = tensor->params.scale;
  int32_t zp = tensor->params.zero_point;
  int32_t q = (int32_t)round(value / scale) + zp;
  if (q < -128) q = -128;
  if (q > 127) q = 127;
  return (int8_t)q;
}

inline float int8_to_float(int8_t q, TfLiteTensor *tensor) {
  return ((float)q - (float)tensor->params.zero_point) * tensor->params.scale;
}

// LED handler
void handle_detection_signal(float prob) {
  if (prob > PRED_THRESHOLD) {
    consecutive_detections++;
  } else {
    consecutive_detections = 0;
  }

  unsigned long now = millis();
  if (consecutive_detections >= DETECT_REQUIRED) {
    last_detection_time = now;
  }

  if ((now - last_detection_time) <= HOLD_MS) {
    if (!led_state) {
      digitalWrite(LED_PIN, HIGH);
      led_state = true;
    }
  } else {
    if (led_state) {
      digitalWrite(LED_PIN, LOW);
      led_state = false;
    }
  }
}

// Run inference on one feature vector
void run_inference_with_feature_vector(float features[], int len) {
  if (len != FEATURE_COUNT) {
    Serial.println("Invalid feature length!");
    return;
  }

  // Fill input tensor
  if (input->type == kTfLiteInt8) {
    for (int i = 0; i < len; i++) {
      float norm = normalize_val(features[i], means[i], scales[i]);
      input->data.int8[i] = float_to_int8(norm, input);
    }
  } else if (input->type == kTfLiteFloat32) {
    for (int i = 0; i < len; i++) {
      float norm = normalize_val(features[i], means[i], scales[i]);
      input->data.f[i] = norm;
    }
  }

  // Inference
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Invoke failed!");
    return;
  }

  // Read output
  float prob = 0.0f;
  if (output->type == kTfLiteInt8) {
    prob = int8_to_float(output->data.int8[0], output);
  } else if (output->type == kTfLiteFloat32) {
    prob = output->data.f[0];
  }

  Serial.print("Model prob = ");
  Serial.println(prob, 6);

  if (prob > PRED_THRESHOLD) {
    Serial.println(">>> Intrusion Detected! <<<");
  } else {
    Serial.println("--- Normal ---");
  }

  // Handle LED alert
  handle_detection_signal(prob);
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n=== ESP32 IDS Demo Starting ===");

  // WiFi AP for sending UDP test packets
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  udp.begin(UDP_PORT);
  Serial.print("Listening on UDP port ");
  Serial.println(UDP_PORT);

  // Load model
  const tflite::Model *model = tflite::GetModel(ids_ann_int8_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1) delay(1000);
  }

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("AllocateTensors failed!");
    while (1) delay(1000);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("Setup complete. Send UDP packets to test.");
}

// ========== LOOP ==========
void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    float features[FEATURE_COUNT] = {0};
    int len = udp.read((char *)features, sizeof(features));
    int count = len / sizeof(float);

    if (count == FEATURE_COUNT) {
      run_inference_with_feature_vector(features, count);
    } else {
      Serial.println("Invalid feature packet received!");
    }
  }
}
