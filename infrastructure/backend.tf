# Remote state backend — the S3 bucket and DynamoDB lock table must be
# bootstrapped first via the ./modules/s3-state module (run once).
#
# The state bucket and lock table names are produced by module.state_backend
# in main.tf and written to terraform.tfvars, so you don't have to type them
# by hand. Fill in state_bucket_name in terraform.tfvars first.
#
# To bootstrap:
#   cd infrastructure
#   terraform init                                # uses local state the first time
#   terraform apply -target=module.state_backend  # creates the S3 bucket + lock table
#   # Then uncomment the block below, replacing the literal values with the
#   # outputs of: terraform output -json state_bucket_name state_lock_table_name
#   terraform init -migrate-state                 # moves state into S3
#
# After bootstrap, the commented block should look like:
#
# terraform {
#   backend "s3" {
#     bucket         = "<value-of-state_bucket_name-output>"
#     key            = "sensor-access/terraform.tfstate"
#     region         = "<value-of-aws_region-variable>"
#     dynamodb_table = "<value-of-state_lock_table_name-output>"
#     encrypt        = true
#   }
# }

# terraform {
#   backend "s3" {
#     bucket         = "<your-tfstate-bucket-name>"
#     key            = "sensor-access/terraform.tfstate"
#     region         = "<your-region>"
#     dynamodb_table = "<your-lock-table-name>"
#     encrypt        = true
#   }
# }
