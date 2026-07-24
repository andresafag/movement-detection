# ── SNS topic for sensor notifications ─────────────────────────────
module "sns" {
  source = "./modules/sns"

  project_name      = var.project_name
  environment       = var.environment
  subscriber_emails = var.sns_subscriber_emails
}

# ── IAM role + policies for IoT Rules Engine ───────────────────────
module "iam" {
  source = "./modules/iam"

  project_name  = var.project_name
  environment   = var.environment
  sns_topic_arn = module.sns.topic_arn
}

# ── IoT Core (thing, cert, policy, Secrets Manager) ────────────────
module "iot_core" {
  source = "./modules/iot-core"

  project_name               = var.project_name
  environment                = var.environment
  aws_region                 = var.aws_region
  thing_name                 = var.thing_name
  topic_prefix               = var.topic_prefix
  aws_s3_bucket_firmware_arn = module.s3.aws_s3_bucket_firmware_arn

}

# ── IoT Topic Rule (MQTT → SNS notification) ───────────────────────
module "iot_rules" {
  source = "./modules/iot-rules"

  project_name      = var.project_name
  environment       = var.environment
  topic_prefix      = var.topic_prefix
  sns_topic_arn     = module.sns.topic_arn
  iot_rule_role_arn = module.iam.iot_rule_engine_role_arn
}

module "s3" {
  source                                          = "./modules/s3"
  aws_lambda_permission_allow_s3_to_invoke_lambda = module.lambda_function.lambda_function_name
  firmware_bucket_name                            = var.firmware_bucket_name
}

module "eventbridge" {
  source = "terraform-aws-modules/eventbridge/aws"

  bus_name = false

  rules = {
    s3_upload_rule = {
      description = "Capture all detection data from esp32 movement sensor"
      enabled     = true
      event_pattern = jsonencode({
        source      = ["aws.s3"]
        detail-type = ["Firmware uploaded"]
        detail = {
          bucket = {
            name = ["my-target-bucket-name"]
          }
        }
      })
    }
  }

  targets = {
    s3_upload_rule = [
      {
        name = "send-data-to esp32-movement-sensor"
        arn  = module.lambda_function.lambda_function_arn
      }
    ]
  }

  tags = {
    Name = "esp32-eventbridge-data"
  }
}
# 1. Statically declare the zip archive generation
data "archive_file" "lambda_zip" {
  type        = "zip"
  source_dir  = "${path.module}/function_lambda"
  output_path = "${path.module}/lambda_function_payload.zip"
}

module "lambda_function" {
  source                                  = "terraform-aws-modules/lambda/aws"
  create_current_version_allowed_triggers = false

  function_name = "firmware-sensor-movement-trigger"
  description   = "Sensor movement lambda function"
  handler       = "index.lambda_handler"
  runtime       = "python3.12"

  allowed_triggers = {
    eventbridge = {
      principal  = "events.amazonaws.com"
      source_arn = module.eventbridge.eventbridge_rule_arns["s3_upload_rule"]
    }
  }

  # 1. Disable inline policy statements (avoids iam:PutRolePolicy)
  attach_policy_statements = false

  # 2. Attach AWS Managed Policies instead (uses iam:AttachRolePolicy)
  attach_policies    = true
  number_of_policies = 2
  policies = [
    "arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole",
    "arn:aws:iam::aws:policy/AmazonS3ReadOnlyAccess"
  ]

  # 3. Tell the module not to create inline CloudWatch log policies or log groups
  use_existing_cloudwatch_log_group = false
  attach_cloudwatch_logs_policy     = false


  create_package         = false
  local_existing_package = data.archive_file.lambda_zip.output_path

  tags = {
    Name = "sensor-movement-esp32"
  }
}