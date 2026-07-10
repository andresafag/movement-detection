locals {
  name_prefix = "${var.project_name}-${var.environment}"
}

# ── IoT Rule Engine Role ───────────────────────────────────────────────────────
# This role is assumed by the AWS IoT Rules Engine (not a user or device)
# to write sensor data into DynamoDB.

data "aws_iam_policy_document" "iot_assume_role" {
  statement {
    sid     = "AllowIoTRulesEngineAssume"
    effect  = "Allow"
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["iot.amazonaws.com"]
    }
  }
}

resource "aws_iam_role" "iot_rule_engine" {
  name               = "${local.name_prefix}-iot-rule-engine-role"
  assume_role_policy = data.aws_iam_policy_document.iot_assume_role.json
  description        = "Role assumed by IoT Rules Engine to write to DynamoDB"

  tags = {
    Name = "${local.name_prefix}-iot-rule-engine-role"
  }
}

# ── DynamoDB Write Policy ─────────────────────────────────────────────────────
# Least-privilege: only PutItem on the specific sensor data table.

data "aws_iam_policy_document" "dynamodb_write" {
  statement {
    sid    = "AllowDynamoDBPutItem"
    effect = "Allow"
    actions = [
      "dynamodb:PutItem",
      "dynamodb:UpdateItem",
    ]
    resources = [var.dynamodb_table_arn]
  }
}

resource "aws_iam_policy" "dynamodb_write" {
  name        = "${local.name_prefix}-iot-dynamodb-write"
  description = "Allows IoT Rules Engine to write sensor data to DynamoDB"
  policy      = data.aws_iam_policy_document.dynamodb_write.json
}

resource "aws_iam_role_policy_attachment" "iot_dynamodb" {
  role       = aws_iam_role.iot_rule_engine.name
  policy_arn = aws_iam_policy.dynamodb_write.arn
}

# ── CloudWatch Logs Policy ────────────────────────────────────────────────────
# Allows IoT Rules Engine to write error logs to CloudWatch.

data "aws_iam_policy_document" "iot_logging" {
  statement {
    sid    = "AllowCloudWatchLogs"
    effect = "Allow"
    actions = [
      "logs:CreateLogGroup",
      "logs:CreateLogStream",
      "logs:PutLogEvents",
    ]
    resources = ["arn:aws:logs:*:*:log-group:/aws/iot/*"]
  }
}

resource "aws_iam_policy" "iot_logging" {
  name        = "${local.name_prefix}-iot-logging"
  description = "Allows IoT Rules Engine to write error logs to CloudWatch"
  policy      = data.aws_iam_policy_document.iot_logging.json
}

resource "aws_iam_role_policy_attachment" "iot_logging" {
  role       = aws_iam_role.iot_rule_engine.name
  policy_arn = aws_iam_policy.iot_logging.arn
}
