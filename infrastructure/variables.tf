variable "aws_region" {
  description = "AWS region to deploy all resources into"
  type        = string
  default     = "us-east-1"
}

variable "project_name" {
  description = "Short name used to prefix all resource names (lowercase, no spaces)"
  type        = string
  default     = "sensor-access"
}

variable "environment" {
  description = "Deployment environment (dev | staging | prod)"
  type        = string
  default     = "dev"

  validation {
    condition     = contains(["dev", "staging", "prod"], var.environment)
    error_message = "environment must be one of: dev, staging, prod."
  }
}

# ── IoT Core ──────────────────────────────────────────────────────────────────

variable "thing_name" {
  description = "Name of the IoT Thing representing the ESP32 device"
  type        = string
  default     = "esp32-sensor-01"
}

variable "topic_prefix" {
  description = "MQTT topic prefix used by the device (no trailing slash)"
  type        = string
  default     = "sensors/motion"
}

# ── SNS ───────────────────────────────────────────────────────────────────────

variable "sns_subscriber_emails" {
  description = "Email addresses that will receive sensor notifications via SNS"
  type        = list(string)
  default = [
    "andresfelipeacostagarcia34@gmail.com", "brigethedith01@gmail.com"
  ]
}

# ── S3 ───────────────────────────────────────────────────────────────────────

variable "firmware_bucket_name" {
  description = "Name of the S3 bucket used to store firmware files for the ESP32 device"
  type        = string
  default     = "esp32-movement-sensor-firmware-bucket"
}