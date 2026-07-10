output "table_name" {
  description = "Name of the DynamoDB sensor data table"
  value       = aws_dynamodb_table.sensor_data.name
}

output "table_arn" {
  description = "ARN of the DynamoDB sensor data table"
  value       = aws_dynamodb_table.sensor_data.arn
}
