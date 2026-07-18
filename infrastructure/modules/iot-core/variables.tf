variable "project_name" {
  description = "Project name prefix"
  type        = string
}

variable "environment" {
  description = "Deployment environment"
  type        = string
}

variable "aws_region" {
  description = "AWS region"
  type        = string
}

variable "thing_name" {
  description = "Name of the IoT Thing (the ESP32 device)"
  type        = string
}

variable "topic_prefix" {
  description = "MQTT topic prefix the device publishes to (e.g. sensors/motion)"
  type        = string
}

variable "aws_s3_bucket_firmware_arn" {
  description = "ARN of the S3 bucket holding firmware binaries"
  type        = string
}
