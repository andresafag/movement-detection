variable "firmware_bucket_name" {
  type        = string
  description = "The name of the firmware"
}

variable "aws_lambda_permission_allow_s3_to_invoke_lambda" {
  type        = string
  description = "The name of the lambda permission to allow S3 to invoke the lambda function"
}
