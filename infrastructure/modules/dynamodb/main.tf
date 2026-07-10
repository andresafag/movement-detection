locals {
  name_prefix = "${var.project_name}-${var.environment}"
}

# ── Sensor Data Table ─────────────────────────────────────────────────────────
# Partition key : device_id  (which thing sent the message)
# Sort key      : timestamp  (ISO-8601 string, enables range queries)

resource "aws_dynamodb_table" "sensor_data" {
  name         = "${local.name_prefix}-sensor-data"
  billing_mode = var.billing_mode
  hash_key     = "device_id"
  range_key    = "timestamp"

  attribute {
    name = "device_id"
    type = "S"
  }

  attribute {
    name = "timestamp"
    type = "S"
  }

  # TTL — automatically expire old records after N days to control costs
  ttl {
    attribute_name = "ttl"
    enabled        = true
  }

  # Point-in-time recovery — protects against accidental deletes/overwrites
  point_in_time_recovery {
    enabled = true
  }

  # Encryption at rest using AWS-managed key
  server_side_encryption {
    enabled = true
  }

  tags = {
    Name = "${local.name_prefix}-sensor-data"
  }
}
