###############################################################################
# Public outputs for the sensor-access infrastructure.
#
# Sensitive values (device certificate PEM, private key) are intentionally
# not re-exported here — they live in AWS Secrets Manager (output below) and
# are marked sensitive in modules/iot-core/outputs.tf.
###############################################################################

# ── VPC ─────────────────────────────────────────────────────────────────
output "vpc_id" {
  description = "ID of the VPC"
  value       = module.vpc.vpc_id
}

output "vpc_cidr" {
  description = "CIDR block of the VPC"
  value       = module.vpc.vpc_cidr
}

output "public_subnet_ids" {
  description = "IDs of the public subnets"
  value       = module.vpc.public_subnet_ids
}

output "private_subnet_ids" {
  description = "IDs of the private subnets"
  value       = module.vpc.private_subnet_ids
}

# ── IoT Core ────────────────────────────────────────────────────────────
output "iot_thing_name" {
  description = "Name of the registered IoT Thing"
  value       = module.iot_core.thing_name
}

output "iot_job_template_arn" {
  description = "ARN de la plantilla enviado a GitHub Actions"
  value       = module.iot_core.iot_job_template_arn
}

output "iot_ota_role_arn" {
  description = "ARN del rol de IAM enviado a GitHub Actions"
  value       = module.iot_core.iot_ota_role_arn
}

output "iot_thing_arn" {
  description = "ARN of the registered IoT Thing"
  value       = module.iot_core.thing_arn
}

output "iot_policy_name" {
  description = "Name of the IoT policy attached to the device certificate"
  value       = module.iot_core.iot_policy_name
}

output "device_cert_secret_arn" {
  description = "ARN of the Secrets Manager secret holding device certificate + keys"
  value       = module.iot_core.secrets_manager_cert_arn
}

output "certificate_device_public_key" {
  description = "ARN of the device certificate"
  value       = module.iot_core.certs_public_key
  sensitive   = true
}

output "certificate_pem" {
  description = "Device certificate PEM (also stored in Secrets Manager)"
  value       = module.iot_core.certificate_pem
  sensitive   = true
}

output "private_key" {
  description = "Device private key (also stored in Secrets Manager)"
  value       = module.iot_core.private_key
  sensitive   = true
}


# ── Data pipeline ───────────────────────────────────────────────────────
output "sns_topic_arn" {
  description = "ARN of the SNS topic receiving sensor notifications"
  value       = module.sns.topic_arn
}

output "sns_topic_name" {
  description = "Name of the SNS topic receiving sensor notifications"
  value       = module.sns.topic_name
}

output "iot_rule_name" {
  description = "Name of the IoT topic rule routing MQTT to SNS"
  value       = module.iot_rules.rule_name
}

output "iot_rule_log_group" {
  description = "CloudWatch log group name for IoT rule error messages"
  value       = module.iot_rules.error_log_group_name
}

# ── State backend ───────────────────────────────────────────────────────
output "state_bucket_name" {
  description = "Name of the S3 bucket holding Terraform remote state"
  value       = module.state_backend.bucket_name
}

output "state_bucket_arn" {
  description = "ARN of the S3 bucket holding Terraform remote state"
  value       = module.state_backend.bucket_arn
}

output "state_lock_table_name" {
  description = "Name of the DynamoDB table used for Terraform state locking"
  value       = module.state_backend.dynamodb_lock_table_name
}


# ── Firmware bucket ───────────────────────────────────────────────────────

output "aws_s3_bucket_firmware_arn" {
  value = module.s3.aws_s3_bucket_firmware_arn
}