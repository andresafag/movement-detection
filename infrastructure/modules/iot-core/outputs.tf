output "thing_arn" {
  description = "ARN of the IoT Thing"
  value       = aws_iot_thing.device.arn
}

output "thing_name" {
  description = "Name of the IoT Thing"
  value       = aws_iot_thing.device.name
}

output "certificate_arn" {
  description = "ARN of the device certificate"
  value       = aws_iot_certificate.device.arn
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
