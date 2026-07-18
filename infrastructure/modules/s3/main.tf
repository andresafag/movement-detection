resource "aws_s3_bucket" "firmware" {
  bucket = var.firmware_bucket_name

  tags = {
    Name = var.firmware_bucket_name
  }
  
}

resource "aws_s3_bucket_versioning" "esp32_version" {
  bucket = var.firmware_bucket_name

  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "tfstate" {
  bucket = aws_s3_bucket.firmware.id

  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "AES256"
    }
  }
}

resource "aws_s3_bucket_public_access_block" "tfstate" {
  bucket = aws_s3_bucket.firmware.id

  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

# Enforce HTTPS-only access to the state bucket
resource "aws_s3_bucket_policy" "esp32_bucket_https_only" {
  bucket = aws_s3_bucket.firmware.id

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Sid       = "DenyHTTP"
        Effect    = "Deny"
        Principal = "*"
        Action    = "s3:*"
        Resource = [
          aws_s3_bucket.firmware.arn,
          "${aws_s3_bucket.firmware.arn}/*",
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