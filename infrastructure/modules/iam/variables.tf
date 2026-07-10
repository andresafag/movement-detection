variable "project_name" {
  description = "Project name prefix"
  type        = string
}

variable "environment" {
  description = "Deployment environment"
  type        = string
}

variable "dynamodb_table_arn" {
  description = "ARN of the DynamoDB table the IoT rule engine will write to"
  type        = string
}
