###############################################################################
# Root composition for sensor-access infrastructure.
#
# Bootstrap order (first time only — see backend.tf):
#   1. terraform init                                  (uses local state)
#   2. terraform apply -target=module.state_backend    (creates the bucket + lock table)
#   3. Uncomment the S3 backend block in backend.tf
#   4. terraform init -migrate-state                   (moves state into S3)
#   5. terraform apply                                 (full stack)
#
# Module dependency order:
#   s3-state → vpc → dynamodb → iam → iot-core → iot-rules
###############################################################################

# ── 1. Remote state backend (bootstrap first) ──────────────────────────
module "state_backend" {
  source = "./modules/s3-state"

  project_name = var.project_name
  environment  = var.environment
  bucket_name  = var.state_bucket_name
}

# ── 2. VPC + endpoints ────────────────────────────────────────────────
module "vpc" {
  source = "./modules/vpc"

  project_name         = var.project_name
  environment          = var.environment
  aws_region           = var.aws_region
  vpc_cidr             = var.vpc_cidr
  public_subnet_cidrs  = var.public_subnet_cidrs
  private_subnet_cidrs = var.private_subnet_cidrs
  availability_zones   = var.availability_zones
}

# ── 3. DynamoDB sensor data table ─────────────────────────────────────
module "dynamodb" {
  source = "./modules/dynamodb"

  project_name = var.project_name
  environment  = var.environment
  billing_mode = var.dynamodb_billing_mode
}

# ── 4. IAM role + policies for IoT Rules Engine ───────────────────────
module "iam" {
  source = "./modules/iam"

  project_name       = var.project_name
  environment        = var.environment
  dynamodb_table_arn = module.dynamodb.table_arn
}

# ── 5. IoT Core (thing, cert, policy, Secrets Manager) ────────────────
module "iot_core" {
  source = "./modules/iot-core"

  project_name = var.project_name
  environment  = var.environment
  aws_region   = var.aws_region
  thing_name   = var.iot_thing_name
  topic_prefix = var.iot_topic_prefix
}

# ── 6. IoT Topic Rule (MQTT → DynamoDB data pipeline) ─────────────────
module "iot_rules" {
  source = "./modules/iot-rules"

  project_name        = var.project_name
  environment         = var.environment
  topic_prefix        = var.iot_topic_prefix
  dynamodb_table_name = module.dynamodb.table_name
  iot_rule_role_arn   = module.iam.iot_rule_engine_role_arn
}
