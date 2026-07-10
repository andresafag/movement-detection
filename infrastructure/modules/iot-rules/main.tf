locals {
  name_prefix = "${var.project_name}-${var.environment}"
}

# ── IoT Topic Rule ────────────────────────────────────────────────────────────
# Triggers on every MQTT message published to the device topic prefix.
# Extracts fields from the JSON payload and writes a row to DynamoDB.
#
# Expected device payload shape:
# {
#   "device_id": "esp32-sensor-01",
#   "timestamp": "2026-07-10T11:00:00Z",
#   "event":     "motion_detected",
#   "ttl":       1757000000        (Unix epoch, optional — for DynamoDB TTL)
# }

resource "aws_iot_topic_rule" "sensor_to_dynamodb" {
  name        = replace("${local.name_prefix}_sensor_data", "-", "_")
  description = "Routes sensor MQTT messages to DynamoDB"
  enabled     = true

  # SQL selects the full message plus the topic for traceability
  sql         = "SELECT *, topic() AS mqtt_topic FROM '${var.topic_prefix}/+'"
  sql_version = "2016-03-23"

  dynamodb {
    table_name = var.dynamodb_table_name
    role_arn   = var.iot_rule_role_arn

    # Maps directly to DynamoDB attribute names
    hash_key_field  = "device_id"
    hash_key_type   = "STRING"
    hash_key_value  = "$${device_id}"   # ${ } is the IoT SQL substitution template

    range_key_field = "timestamp"
    range_key_type  = "STRING"
    range_key_value = "$${timestamp}"

    # Stores the full JSON payload as a single attribute for easy retrieval
    payload_field = "payload"
  }

  # Dead-letter queue: if the DynamoDB action fails, log the error to CloudWatch
  error_action {
    cloudwatch_logs {
      log_group_name = aws_cloudwatch_log_group.iot_rule_errors.name
      role_arn       = var.iot_rule_role_arn
    }
  }

  tags = {
    Name = "${local.name_prefix}-sensor-rule"
  }
}

# ── CloudWatch Log Group for rule errors ──────────────────────────────────────

resource "aws_cloudwatch_log_group" "iot_rule_errors" {
  name              = "/aws/iot/${local.name_prefix}/rule-errors"
  retention_in_days = 14

  tags = {
    Name = "${local.name_prefix}-iot-rule-errors"
  }
}
