locals {
  name_prefix = "${var.project_name}-${var.environment}"
}

# ── S3 Bucket for Terraform State ─────────────────────────────────────────────

resource "aws_s3_bucket" "tfstate" {
  bucket = var.bucket_name

  # Prevent accidental destruction of state bucket
  lifecycle {
    prevent_destroy = true
  }

  tags = {
    Name = "${local.name_prefix}-tfstate"
  }
}

resource "aws_s3_bucket_versioning" "tfstate" {
  bucket = aws_s3_bucket.tfstate.id

  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "tfstate" {
  bucket = aws_s3_bucket.tfstate.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_public_access_block" "tfstate" {
  bucket = aws_s3_bucket.tfstate.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

# Enforce HTTPS-only access to the state bucket
resource "aws_s3_bucket_policy" "tfstate_https_only" {
  bucket = aws_s3_bucket.tfstate.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Sid       = "DenyHTTP"
        Effect    = "Deny"
        Principal = "*"
        Action    = "s3:*"
        Resource = [
          aws_s3_bucket.tfstate.arn,
          "${aws_s3_bucket.tfstate.arn}/*",
        ]
        Condition = {
          Bool = {
            "aws:SecureTransport" = "false"
          }
        }
      }
    ]
  })
}

# ── DynamoDB Lock Table ───────────────────────────────────────────────────────
# Prevents concurrent Terraform runs from corrupting state.

resource "aws_dynamodb_table" "tfstate_lock" {
  name         = "${local.name_prefix}-tfstate-lock"
  billing_mode = "PAY_PER_REQUEST"
  hash_key     = "LockID"

  attribute {
    name = "LockID"
    type = "S"
  }

  server_side_encryption {
    enabled = true
  }

  tags = {
    Name = "${local.name_prefix}-tfstate-lock"
  }
}
