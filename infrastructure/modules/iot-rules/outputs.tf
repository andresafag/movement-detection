output "rule_name" {
  description = "Name of the IoT topic rule"
  value       = aws_iot_topic_rule.sensor_to_dynamodb.name
}

output "rule_arn" {
  description = "ARN of the IoT topic rule"
  value       = aws_iot_topic_rule.sensor_to_dynamodb.arn
}

output "error_log_group_name" {
  description = "CloudWatch log group name for IoT rule errors"
  value       = aws_cloudwatch_log_group.iot_rule_errors.name
}
