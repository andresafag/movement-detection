locals {
  name_prefix = "${var.project_name}-${var.environment}"
  
  job_document = {
    operation    = "ota_update"
    version      = "$${aws:iot:parameter:firmware_version}"
    firmware_url = "https://$${aws:iot:s3-id}/mi-bucket-firmware-esp32/firmware_v$${aws:iot:parameter:firmware_version}.bin"
  }
}

terraform {
  required_providers {
    awscc = {
      source  = "hashicorp/awscc"
      version = "~> 1.0"
    }
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

# ── IoT Thing ─────────────────────────────────────────────────────────────────

resource "aws_iot_thing" "device" {
  name = var.thing_name

  attributes = {
    project     = var.project_name
    environment = var.environment
    firmware    = "esp-idf"
  }
}

# ── Thing Type ────────────────────────────────────────────────────────────────

resource "aws_iot_thing_type" "sensor" {
  name = "${local.name_prefix}-motion-sensor"

  properties {
    description           = "ESP32 PIR motion sensor device"
    searchable_attributes = ["project", "environment"]
  }
}

# ── IoT group ────────────────────────────────────────────────────────────────

resource "aws_iot_thing_group" "ESP32_group" {
  name = "production-devices"

  properties {
    description = "Group for production IoT edge devices"
    
    attribute_payload {
      attributes = {
        device_type       = "ESP32"
        hardware_revision = "v2.1"
        environment       = "production"
      }
    }
  }


  tags = {
    Project = "IoT-Deployment"
  }
}

resource "aws_iot_topic_rule" "telemetry_esp32" {
  name        = "ProcessDeviceTelemetry"
  description = "Routes temperature telemetry data to Timestream"
  enabled     = true
  
  # The SQL statement that filters incoming device telemetry payload
  sql         = "SELECT * FROM ${var.topic_prefix}"
  sql_version = "2016-03-23"

}


# ── IoT job Template ────────────────────────────────────────────────────────────────
resource "awscc_iot_job_template" "esp32_ota_template" {
  job_template_id = "esp32-ota-update-template"
  description     = "Template for OTA firmware updates to ESP32 devices"

  document = <<EOF
{
  "operation": "ota_update",
  "version": "$${aws:iot:parameter:firmware_version}",
  "firmware_url": "$${aws:iot:parameter:firmware_url}"
}
EOF

  presigned_url_config = {
    role_arn       = aws_iam_role.iot-ota-role.arn
    expires_in_sec = 3600 
  }

  job_executions_rollout_config = {
    maximum_per_minute = 10
  }

  timeout_config = {
    in_progress_timeout_in_minutes = 5
  }

  tags = [{
    key   = "Modified By"
    value = "AWSCC"
  }]

  # Modifica este bloque para asegurar el orden de creación estricto:
  depends_on = [
    aws_iam_role.iot-ota-role,
    aws_iam_role_policy.iot-ota-policy
  ]
}

# ── Certificate ───────────────────────────────────────────────────────────────
# Terraform creates the cert and outputs the PEM/key so you can flash them
# onto the device. Store them in AWS Secrets Manager (see iam module).

resource "aws_iot_certificate" "device" {
  active = true
}

# ── Attach certificate to thing ───────────────────────────────────────────────

resource "aws_iot_thing_principal_attachment" "device" {
  thing     = aws_iot_thing.device.name
  principal = aws_iot_certificate.device.arn
}

# ── IoT Policy ────────────────────────────────────────────────────────────────
# Least-privilege: device can only connect with its own client ID,
# subscribe/publish only on its own topic prefix.

data "aws_iam_policy_document" "iot_device_policy" {
  
  # 1. Permitir conexión (Debe coincidir estrictamente con el Client ID / Thing Name)
  statement {
    sid     = "AllowConnect"
    effect  = "Allow"
    actions = ["iot:Connect"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:client/${var.thing_name}"
    ]
  }

  # 2. Permisos de Suscripción para AWS IoT Jobs (Usa topicfilter)
  statement {
    sid       = "AllowIoTJobsSubscribe"
    effect    = "Allow"
    actions   = ["iot:Subscribe"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:topicfilter/$aws/things/${var.thing_name}/jobs/*"
    ]
  }

  # 3. Permisos de Publicación y Recepción para AWS IoT Jobs (Usa topic)
  statement {
    sid       = "AllowIoTJobsPublishReceive"
    effect    = "Allow"
    actions   = ["iot:Publish", "iot:Receive"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:topic/$aws/things/${var.thing_name}/jobs/*"
    ]
  }

  # 4. Permisos de Suscripción para tu Telemetría propia (Usa topicfilter)
  statement {
    sid       = "AllowTelemetrySubscribe"
    effect    = "Allow"
    actions   = ["iot:Subscribe"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:topicfilter/${var.topic_prefix}/*"
    ]
  }

  # 5. Permisos de Publicación y Recepción para tu Telemetría propia (Usa topic)
  statement {
    sid       = "AllowTelemetryPublishReceive"
    effect    = "Allow"
    actions   = ["iot:Publish", "iot:Receive", "iot:RetainPublish"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:topic/${var.topic_prefix}/*"
    ]
  }
}

resource "aws_iot_policy" "device" {
  name   = "${local.name_prefix}-device-policy"
  policy = data.aws_iam_policy_document.iot_device_policy.json
}


resource "aws_iam_role" "iot-ota-role" {
  name = "${local.name_prefix}-iot-ota-role"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Principal = {
          Service = "iot.amazonaws.com"
        }
        Action = "sts:AssumeRole"
      }
    ]
  })
  
}

resource "aws_iam_role_policy" "iot-ota-policy" {
  name = "${local.name_prefix}-iot-ota-policy"
  role = aws_iam_role.iot-ota-role.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "s3:GetObject",
          "s3:ListBucket"
        ]
        Resource = ["${var.aws_s3_bucket_firmware_arn}/*"]
      },
       {
        Effect = "Allow"
        Action = [
          "s3:GetBucketLocation"
        ]
        Resource = ["${var.aws_s3_bucket_firmware_arn}"]
      }
    ]
  })
}

# ── Attach policy to certificate ──────────────────────────────────────────────

resource "aws_iot_policy_attachment" "device" {
  policy = aws_iot_policy.device.name
  target = aws_iot_certificate.device.arn
}


# ── Store certificates in Secrets Manager ─────────────────────────────────────
# Never write certs to disk or commit them. Pull from Secrets Manager
# when flashing the device, or use AWS IoT Fleet Provisioning.

resource "aws_secretsmanager_secret" "device_cert" {
  name                    = "${local.name_prefix}/${var.thing_name}/certificate"
  description             = "IoT device certificate PEM for ${var.thing_name}"
  recovery_window_in_days = 7

  tags = {
    Name = "${local.name_prefix}-device-cert"
  }
}

resource "aws_secretsmanager_secret_version" "device_cert" {
  secret_id = aws_secretsmanager_secret.device_cert.id
  secret_string = jsonencode({
    certificate_pem = aws_iot_certificate.device.certificate_pem
    public_key      = aws_iot_certificate.device.public_key
    private_key     = aws_iot_certificate.device.private_key
  })
}

# ── Data sources ──────────────────────────────────────────────────────────────

data "aws_caller_identity" "current" {}

# ── 1. Exportar la Clave Privada del Dispositivo ──────────────────────────────
resource "local_file" "private_key" {
  content  = aws_iot_certificate.device.private_key
  filename = "${path.root}/../components/mqtt-aws/certs/private.pem.key"
}

# ── 2. Exportar el Certificado PEM del Dispositivo ────────────────────────────
resource "local_file" "pem_key" {
  content  = aws_iot_certificate.device.certificate_pem
  filename = "${path.root}/../components/mqtt-aws/certs/certificate.pem.crt"
}
