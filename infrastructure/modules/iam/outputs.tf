output "iot_rule_engine_role_arn" {
  description = "ARN of the IAM role assumed by the IoT Rules Engine"
  value       = aws_iam_role.iot_rule_engine.arn
}

output "iot_rule_engine_role_name" {
  description = "Name of the IAM role assumed by the IoT Rules Engine"
  value       = aws_iam_role.iot_rule_engine.name
}
