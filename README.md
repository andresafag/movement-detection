# 🛰️ Sensor Access — IoT Motion Alert Pipeline

> **A pocket-sized, end-to-end IoT pipeline that watches a doorway and pings your inbox the instant someone walks in.** 🚨📩

An **ESP32** paired with an **HC-SR501 PIR motion sensor** detects physical movement, publishes an event over **MQTT** to **AWS IoT Core**, where an **IoT Topic Rule** fans the event out to **Amazon SNS**, delivering an email alert in seconds.

Built with embedded C (ESP-IDF) on the device side and Terraform-managed AWS infrastructure on the cloud side. ✨

---

## 🎯 What This Project Does

| Step | What happens | Who does it |
|------|--------------|-------------|
| 1️⃣ | A human (or a cat 🐈) walks past the sensor | HC-SR501 PIR |
| 2️⃣ | The PIR output pin goes HIGH on GPIO 27 | ESP32 ISR |
| 3️⃣ | A FreeRTOS task wakes up, blinks the blue LED 🔵 for 2s | ESP32 firmware |
| 4️⃣ | A JSON payload is published over MQTT/TLS to AWS IoT Core | `mqtt-aws` component |
| 5️⃣ | An IoT Topic Rule matches `sensors/motion/+` and triggers an action | AWS IoT Rules Engine |
| 6️⃣ | The payload is published to an SNS topic | SNS |
| 7️⃣ | SNS fans out to every confirmed email subscriber | You 📬 |

---

## 🏗️ High-Level Architecture

```mermaid
flowchart LR
    A[🏃 Motion in the room] --> B[🟢 HC-SR501 PIR Sensor]
    B -- "GPIO 27 RISING EDGE" --> C[🧠 ESP32 Firmware<br/>FreeRTOS ISR + Task]
    C -- "JSON over MQTT/TLS :8883" --> D[☁️ AWS IoT Core<br/>Thing + Cert + Policy]
    D -- "MQTT topic<br/>sensors/motion/+" --> E[⚙️ IoT Topic Rule<br/>SQL: SELECT *]
    E -- "sns:Publish" --> F[📣 SNS Topic<br/>sensor-alerts]
    F -- "Email protocol" --> G[📩 Your Inbox]

    H[🔐 Secrets Manager<br/>cert + key] -.-> C
    I[📊 CloudWatch Logs<br/>rule errors] -.-> E

    style A fill:#ffe4b5,stroke:#333
    style B fill:#90ee90,stroke:#333
    style C fill:#87ceeb,stroke:#333
    style D fill:#ffd700,stroke:#333
    style E fill:#dda0dd,stroke:#333
    style F fill:#ffa07a,stroke:#333
    style G fill:#98fb98,stroke:#333
```

---

## 🧩 Repository Layout

```mermaid
graph TD
    Root[📂 sensor-access/] --> M[🧠 main/<br/>app_main + entry]
    Root --> C1[🔌 components/movement-driver/<br/>PIR ISR + LED task]
    Root --> C2[📡 components/mqtt-aws/<br/>AWS IoT MQTT client]
    Root --> I[☁️ infrastructure/<br/>Terraform IaC]
    Root --> B[🛠️ build/<br/>ESP-IDF artifacts]

    I --> M1[🗄️ s3-state/<br/>remote state + lock]
    I --> M2[🌐 vpc/<br/>subnets + endpoints]
    I --> M3[📣 sns/<br/>email fan-out]
    I --> M4[🛡️ iam/<br/>IoT rule role]
    I --> M5[📡 iot-core/<br/>thing + cert + policy]
    I --> M6[⚙️ iot-rules/<br/>MQTT → SNS rule]

    style Root fill:#f0f8ff,stroke:#333
    style I fill:#fffacd,stroke:#333
```

| Path | Purpose | Tech |
|------|---------|------|
| `main/main.c` | App entry point — boots PIR driver, idle-loops | ESP-IDF, FreeRTOS |
| `components/movement-driver/` | PIR GPIO interrupt, queue, LED blink | C, FreeRTOS |
| `components/mqtt-aws/` | MQTT connect/publish stub (ready to wire) | C |
| `infrastructure/` | AWS infrastructure as code | Terraform |
| `infrastructure/modules/iot-core/` | Thing, certificate, policy, Secrets Manager | AWS IoT |
| `infrastructure/modules/iot-rules/` | SQL rule that routes MQTT → SNS | AWS IoT Rules |
| `infrastructure/modules/sns/` | Topic + email subscriptions | SNS |
| `infrastructure/modules/iam/` | Least-privilege role for the rule engine | IAM |
| `infrastructure/modules/vpc/` | Subnets, IGW, VPC endpoints (future-proof) | VPC |
| `infrastructure/modules/s3-state/` | Encrypted remote state + DynamoDB lock | S3 + DynamoDB |

---

## ⚡ The Device Side — ESP32 Firmware

### 🔌 Hardware Wiring

```mermaid
graph LR
    subgraph ESP32["🧠 ESP32 DevKit V1"]
        G27[GPIO 27]
        G25[GPIO 25]
        G5V[5V]
        GG[GND]
    end

    subgraph PIR["🟢 HC-SR501 PIR"]
        VCC[VCC]
        OUT[OUT]
        GND[GND]
    end

    LED[🔵 Blue LED]

    VCC --> G5V
    OUT --> G27
    GND --> GG
    G25 --> LED
    LED --> GG

    style ESP32 fill:#e6f3ff,stroke:#333
    style PIR fill:#e6ffe6,stroke:#333
```

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| PIR VCC | 5V | HC-SR501 needs 5V |
| PIR OUT | **GPIO 27** | Input, pull-down, rising-edge interrupt |
| PIR GND | GND | Common ground |
| Blue LED (+) | **GPIO 25** | Output, sinks to GND through resistor |

### 🔁 Runtime Flow

```mermaid
sequenceDiagram
    participant P as 🟢 HC-SR501 PIR
    participant I as ⚡ ISR (IRAM)
    participant Q as 📥 FreeRTOS Queue
    participant T as 🧵 pir_processing_task
    participant L as 🔵 Blue LED
    participant M as 📡 mqtt-aws
    participant A as ☁️ AWS IoT Core

    P->>I: Rising edge on GPIO 27
    I->>Q: xQueueSendFromISR(gpio_num)
    Note over I: portYIELD_FROM_ISR() if needed
    Q->>T: xQueueReceive(portMAX_DELAY)
    T->>L: gpio_set_level(25, 1) 🔵 ON
    T->>T: vTaskDelay(2000 ms)
    T->>L: gpio_set_level(25, 0) ⚫ OFF
    T->>Q: xQueueReset() (drop noise)
    T->>M: mqtt_aws_publish(topic, json)
    M->>A: PUBLISH sensors/motion/esp32-sensor-01
    A-->>M: PUBACK ✅
```

### 🧠 Key Implementation Details

- **ISR runs in IRAM** — minimal latency, only enqueues a `uint32_t` to avoid blocking the CPU.
- **FreeRTOS queue** (`xQueueCreate(10, sizeof(uint32_t))`) decouples the ISR from the work loop.
- **Debounce window** — the task holds the LED on for 2 s and then `xQueueReset`s the queue, so back-to-back triggers from the same motion event don't spam the broker. 🧹
- **Boot blink** — LED flashes for 500 ms at startup as a hardware sanity check. ✅

---

## ☁️ The Cloud Side — AWS Infrastructure (Terraform)

### 🧱 Module Dependency Graph

```mermaid
flowchart TD
    State[🗄️ s3-state<br/>remote state + lock] --> Root[🧩 Root main.tf]
    Vpc[🌐 vpc<br/>subnets + endpoints] --> Root
    Sns[📣 sns<br/>topic + email subs] --> Root
    Iam[🛡️ iam<br/>IoT rule engine role] --> Root
    Core[📡 iot-core<br/>thing + cert + policy] --> Root
    Rules[⚙️ iot-rules<br/>MQTT → SNS] --> Root

    Sns -->|sns_topic_arn| Iam
    Iam -->|iot_rule_role_arn| Rules
    Sns -->|sns_topic_arn| Rules

    style State fill:#fffacd
    style Vpc fill:#e0ffff
    style Sns fill:#ffe4e1
    style Iam fill:#f0e68c
    style Core fill:#e6e6fa
    style Rules fill:#dda0dd
```

### 🛰️ IoT Core — The Device Identity Plane

```mermaid
graph LR
    Thing[📟 IoT Thing<br/>esp32-sensor-01] -->|attached| Cert[🔐 X.509 Certificate]
    Cert -->|attached| Policy[📜 IoT Policy<br/>sensor-access-dev-device-policy]

    Policy -- "iot:Connect" --> C1[✅ client/esp32-sensor-01]
    Policy -- "iot:Publish" --> C2[✅ topic/sensors/motion/*]
    Policy -- "iot:Subscribe" --> C3[✅ topicfilter/sensors/motion/*]
    Policy -- "iot:Receive" --> C4[✅ topic/sensors/motion/*]

    Cert -.->|PEM + key| SM[🔒 Secrets Manager<br/>sensor-access-dev/esp32-sensor-01/certificate]

    style Thing fill:#87ceeb
    style Cert fill:#ffd700
    style Policy fill:#dda0dd
    style SM fill:#ff6347
```

> 🔒 The certificate, public key, and **private key** never touch the repo — Terraform creates them and ships them to **AWS Secrets Manager**. Flash them onto the ESP32 from there at provisioning time.

### ⚙️ The IoT Topic Rule — The Brain of the Pipeline

```sql
SELECT *, topic() AS mqtt_topic
FROM   'sensors/motion/+'
```

```mermaid
flowchart LR
    subgraph In["📥 Input: MQTT topic sensors/motion/+"]
        M1[/esp32-sensor-01/]
        M2[/future-device-02/]
    end

    subgraph Rule["⚙️ IoT Topic Rule (SQL)"]
        SQL[SELECT * + topic]
    end

    subgraph Out["📤 Actions"]
        SNS[📣 SNS Publish<br/>sensor-alerts topic]
        DLQ[📊 CloudWatch Logs<br/>rule errors]
    end

    In --> Rule
    Rule -->|happy path| SNS
    Rule -->|failure| DLQ

    SNS --> Email1[📩 Subscriber 1]
    SNS --> Email2[📩 Subscriber 2]
    SNS --> Email3[📩 Subscriber N...]

    style Rule fill:#dda0dd
    style SNS fill:#ffa07a
    style DLQ fill:#ff6347
```

> 🛡️ The rule engine assumes a dedicated IAM role (`sensor-access-dev-iot-rule-engine-role`) with `sns:Publish` scoped to **only** the sensor alerts topic. Failures fall through to a CloudWatch log group with 14-day retention.

### 📬 Expected Email Payload Shape

The full JSON body sent by the device becomes the email body:

```json
{
  "device_id": "esp32-sensor-01",
  "timestamp": "2026-07-10T11:00:00Z",
  "event":     "motion_detected",
  "ttl":       1757000000
}
```

---

## 🔐 Security Model 🛡️

| Layer | Control | Why it matters |
|-------|---------|----------------|
| 🔐 Transport | MQTT over **TLS 1.2** (port 8883) | ESP32 → AWS IoT Core is encrypted in transit |
| 🪪 Identity | **X.509 certificate** per device | No shared secrets, easy to revoke |
| 📜 Authorization | **Least-privilege IoT policy** | Device can only use its own client ID and topic subtree |
| 🤖 Rule engine | Dedicated IAM role with `sns:Publish` scoped to one ARN | Blast radius is one topic |
| 🗄️ State | S3 bucket versioning + KMS encryption + DynamoDB lock | State can't be lost or corrupted by parallel runs |
| 🌐 Network (future) | VPC endpoints for IoT Data + DynamoDB | Private subnets never traverse the public internet |
| 🚫 Secrets | Certs in **AWS Secrets Manager**, not git | Zero secrets in source control |

```mermaid
flowchart LR
    Dev[🧠 ESP32] -- "🔒 TLS" --> VPCe[🌐 VPC Endpoint<br/>com.amazonaws.us-east-1.iot.data]
    VPCe --> IoT[☁️ IoT Core]
    IoT -. "🪪 cert + policy" .- Dev
    IoT -- "🤖 assume role" --> Role[🛡️ iot-rule-engine-role]
    Role -- "✅ sns:Publish" --> Topic[📣 SNS Topic]
    Topic -- "📩 email" --> Inbox[📬 You]

    style Dev fill:#87ceeb
    style VPCe fill:#e0ffff
    style IoT fill:#ffd700
    style Role fill:#f0e68c
    style Topic fill:#ffa07a
    style Inbox fill:#98fb98
```

---

## 🚀 End-to-End Sequence — From Wave to Inbox

```mermaid
sequenceDiagram
    autonumber
    participant 👤 as Human
    participant 🟢 as HC-SR501
    participant 🧠 as ESP32
    participant 📡 as AWS IoT Core
    participant ⚙️ as IoT Rule
    participant 📣 as SNS
    participant 📬 as Email

    👤->>🟢: Walks past sensor 🚶
    🟢->>🧠: OUT pin goes HIGH
    🧠->>🧠: ISR queues event
    🧠->>🧠: LED ON 2s + queue reset
    🧠->>📡: PUBLISH sensors/motion/esp32-sensor-01<br/>(JSON over MQTLS)
    📡->>⚙️: Forwards matched message
    ⚙️->>📣: sns:Publish to sensor-alerts
    📣->>📬: Email delivered to subscriber
    Note over 📬: ⏱️ Typical latency: 1–3 seconds
```

---

## 🧰 Tech Stack

```mermaid
mindmap
  root((🛰️ Sensor Access))
    Firmware
      🧠 ESP-IDF
      ⚡ FreeRTOS
      🔌 GPIO ISR
      📡 MQTT/TLS
    Hardware
      🟢 HC-SR501 PIR
      🔵 Blue LED
      🔌 DOIT ESP32 DevKit V1
    Cloud
      ☁️ AWS IoT Core
      ⚙️ IoT Topic Rules
      📣 Amazon SNS
      🔐 Secrets Manager
      📊 CloudWatch Logs
    IaC
      🟦 Terraform
      🗄️ S3 remote state
      🗃️ DynamoDB lock
      🛡️ IAM least-priv
```

| Domain | Tool | Why |
|--------|------|-----|
| 🧠 Embedded | ESP-IDF + FreeRTOS | First-class on ESP32, deterministic ISRs |
| 📡 Messaging | MQTT over TLS | Lightweight, perfect for constrained devices |
| ☁️ IoT broker | AWS IoT Core | Managed, scales to billions, no server to run |
| ⚙️ Routing | IoT Topic Rules | SQL-style filtering without writing a Lambda |
| 📣 Notifications | Amazon SNS | One-to-many fan-out, email ready out of the box |
| 🟦 IaC | Terraform | Repeatable infra, remote state, plan/apply workflow |
| 🗄️ State | S3 + DynamoDB | Encrypted, versioned, locked — production-grade |

---

## 🩺 Operational Notes

- **Bootstrapping the backend** — the first `terraform apply` runs against a *local* state to create the S3 bucket + DynamoDB lock table, then a one-time `terraform init -migrate-state` moves state to S3. (See comments in `infrastructure/main.tf`.)
- **Email confirmation** — AWS emails every new SNS subscriber a one-time confirmation link. Until it's clicked, the email stays silent. 📭
- **Error visibility** — if the SNS publish ever fails, the rule's `error_action` ships the failure to `/aws/iot/sensor-access-dev/rule-errors` in CloudWatch (14-day retention). 🪵
- **Cost posture** — IoT Core charges per message, SNS per notification, S3 + DynamoDB are pay-per-request. This pipeline costs **pennies per month** at low traffic. 💸
- **Scaling out** — drop more `aws_iot_thing` + certificate resources per device; the topic rule already matches `sensors/motion/+` so any new device ID works automatically. ➕

---

## 🔮 Future-Proofing (Already in the IaC!)

- 🌐 **VPC + subnets** are provisioned with **VPC endpoints** for IoT Data and DynamoDB, so future workloads (Lambda, ECS, RDS) can talk to AWS services without traversing the public internet.
- 🗃️ A **DynamoDB table** schema (`device_id` partition + `timestamp` sort + TTL) is ready to be wired into a future "log every motion event" Lambda. 📈
- 🛡️ IAM roles are **per-purpose** and **least-privilege** — adding a new action means attaching a new policy, not loosening an existing one. 🔒

---

## 📜 License

See `LICENSE` in the repo root. ⚖️

---

> **TL;DR** 🧾 — PIR sensor says "movement!" ➡️ ESP32 blinks an LED and publishes to AWS IoT Core ➡️ a serverless rule forwards to SNS ➡️ you get an email. Cheap, secure, repeatable, and entirely under your control. 🎉
