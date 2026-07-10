locals {
  name_prefix = "${var.project_name}-${var.environment}"
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
    description            = "ESP32 PIR motion sensor device"
    searchable_attributes  = ["project", "environment"]
  }
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
  # Allow the device to connect using only its thing name as client ID
  statement {
    sid    = "AllowConnect"
    effect = "Allow"
    actions = ["iot:Connect"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:client/${var.thing_name}"
    ]
  }

  # Allow publish only to the device's own topic subtree
  statement {
    sid    = "AllowPublish"
    effect = "Allow"
    actions = ["iot:Publish", "iot:RetainPublish"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:topic/${var.topic_prefix}/*"
    ]
  }

  # Allow subscribe + receive on the device's command topic (cloud → device)
  statement {
    sid    = "AllowSubscribe"
    effect = "Allow"
    actions = ["iot:Subscribe"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:topicfilter/${var.topic_prefix}/*"
    ]
  }

  statement {
    sid    = "AllowReceive"
    effect = "Allow"
    actions = ["iot:Receive"]
    resources = [
      "arn:aws:iot:${var.aws_region}:${data.aws_caller_identity.current.account_id}:topic/${var.topic_prefix}/*"
    ]
  }
}

resource "aws_iot_policy" "device" {
  name   = "${local.name_prefix}-device-policy"
  policy = data.aws_iam_policy_document.iot_device_policy.json
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
