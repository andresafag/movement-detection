output "thing_arn" {
  description = "ARN of the IoT Thing"
  value       = aws_iot_thing.device.arn
}

output "iot_job_template_arn" {
  description = "ARN de la plantilla creada por AWSCC"
  value       = awscc_iot_job_template.esp32_ota_template.id
}

output "iot_ota_role_arn" {
  description = "ARN del rol de IAM para las URLs de S3"
  value       = aws_iam_role.iot-ota-role.arn
}

output "thing_name" {
  description = "Name of the IoT Thing"
  value       = aws_iot_thing.device.name
}

output "certificate_arn" {
  description = "ARN of the device certificate"
  value       = aws_iot_certificate.device.arn
}

output "certs_public_key" {
  description = "public key"
  value       = aws_iot_certificate.device.public_key
}

output "certificate_pem" {
  description = "Device certificate PEM (also stored in Secrets Manager)"
  value       = aws_iot_certificate.device.certificate_pem
  sensitive   = true
}

output "private_key" {
  description = "Device private key (also stored in Secrets Manager)"
  value       = aws_iot_certificate.device.private_key
  sensitive   = true
}

output "iot_policy_name" {
  description = "Name of the IoT policy attached to the device certificate"
  value       = aws_iot_policy.device.name
}

output "secrets_manager_cert_arn" {
  description = "ARN of the Secrets Manager secret holding device credentials"
  value       = aws_secretsmanager_secret.device_cert.arn
}
