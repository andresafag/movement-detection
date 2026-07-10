output "bucket_name" {
  description = "Name of the S3 state bucket"
  value       = aws_s3_bucket.tfstate.bucket
}

output "bucket_arn" {
  description = "ARN of the S3 state bucket"
  value       = aws_s3_bucket.tfstate.arn
}

output "dynamodb_lock_table_name" {
  description = "Name of the DynamoDB state lock table"
  value       = aws_dynamodb_table.tfstate_lock.name
}
