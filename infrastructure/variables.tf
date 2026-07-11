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

# ── VPC ───────────────────────────────────────────────────────────────────────

variable "vpc_cidr" {
  description = "CIDR block for the VPC"
  type        = string
  default     = "10.0.0.0/16"
}

variable "public_subnet_cidrs" {
  description = "CIDR blocks for public subnets (one per AZ)"
  type        = list(string)
  default     = ["10.0.1.0/24", "10.0.2.0/24"]
}

variable "private_subnet_cidrs" {
  description = "CIDR blocks for private subnets (one per AZ)"
  type        = list(string)
  default     = ["10.0.11.0/24", "10.0.12.0/24"]
}

variable "availability_zones" {
  description = "Availability zones to use (must match subnet lists length)"
  type        = list(string)
  default     = ["us-east-1a", "us-east-1b"]
}

# ── IoT Core ──────────────────────────────────────────────────────────────────

variable "iot_thing_name" {
  description = "Name of the IoT Thing representing the ESP32 device"
  type        = string
  default     = "esp32-sensor-01"
}

variable "iot_topic_prefix" {
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

# ── State backend ─────────────────────────────────────────────────────────────

variable "state_bucket_name" {
  description = "Globally unique S3 bucket name for Terraform remote state"
  type        = string
  # Override in terraform.tfvars — must be globally unique
  default = "movement-sensor-tfstate"
}
