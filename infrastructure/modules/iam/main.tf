locals {
  name_prefix = "${var.project_name}-${var.environment}"
}

# ── IoT Rule Engine Role ───────────────────────────────────────────────────────
# This role is assumed by the AWS IoT Rules Engine (not a user or device)
# to publish sensor notifications to SNS.

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
  description        = "Role assumed by IoT Rules Engine to publish to SNS"

  tags = {
    Name = "${local.name_prefix}-iot-rule-engine-role"
  }
}

# ── SNS Publish Policy ────────────────────────────────────────────────────────
# Least-privilege: only Publish on the specific sensor alerts topic.

data "aws_iam_policy_document" "sns_publish" {
  statement {
    sid       = "AllowSNSPublish"
    effect    = "Allow"
    actions   = ["sns:Publish"]
    resources = [var.sns_topic_arn]
  }
}

resource "aws_iam_policy" "sns_publish" {
  name        = "${local.name_prefix}-iot-sns-publish"
  description = "Allows IoT Rules Engine to publish sensor notifications to SNS"
  policy      = data.aws_iam_policy_document.sns_publish.json
}

resource "aws_iam_role_policy_attachment" "iot_sns" {
  role       = aws_iam_role.iot_rule_engine.name
  policy_arn = aws_iam_policy.sns_publish.arn
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

# ──  ────────────────────────────────────────────────────
