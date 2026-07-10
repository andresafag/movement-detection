variable "project_name" {
  description = "Project name prefix"
  type        = string
}

variable "environment" {
  description = "Deployment environment"
  type        = string
}

variable "topic_prefix" {
  description = "MQTT topic prefix to subscribe to (e.g. sensors/motion)"
  type        = string
}

variable "dynamodb_table_name" {
  description = "Name of the DynamoDB table to write sensor data into"
  type        = string
}

variable "iot_rule_role_arn" {
  description = "ARN of the IAM role the IoT Rules Engine will assume"
  type        = string
}
