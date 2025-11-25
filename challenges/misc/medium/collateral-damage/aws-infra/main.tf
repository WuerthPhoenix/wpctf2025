terraform {
  required_version = ">= 1.9.6"

  backend "s3" {
    bucket = "wp-red-team-infrastructure-972364" 
    key    = "lambda-ssti-wpctf2025"
    region = "eu-north-1"
    encrypt = true
  }

  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "= 5.68.0"
    }
  }
}

provider "aws" {
  region = "eu-west-1"
}

resource "aws_iam_role" "lambda_role" {
  name = "lambda_s3_ctf_role"
  assume_role_policy = jsonencode({
    Version = "2012-10-17",
    Statement = [
      {
        Action = "sts:AssumeRole",
        Effect = "Allow",
        Principal = {
          Service = "lambda.amazonaws.com"
        }
      }
    ]
  })
}

resource "aws_iam_policy" "lambda_policy" {
  name = "lambda_s3_ctf_policy"
  policy = jsonencode({
    Version = "2012-10-17",
    Statement = [
      {
        Effect = "Allow",
        Action = [
          "s3:ListBucket"
        ],
        Resource = "arn:aws:s3:::wpctf-2025-flag-s3-bucket-0197643419247650"
      },
      {
        Effect = "Allow",
        Action = [
          "s3:GetObject"
        ],
        Resource = "arn:aws:s3:::wpctf-2025-flag-s3-bucket-0197643419247650/*"
      }
    ]
  })
}

resource "aws_iam_role_policy_attachment" "lambda_policy_attach" {
  role       = aws_iam_role.lambda_role.name
  policy_arn = aws_iam_policy.lambda_policy.arn
}

resource "aws_s3_bucket" "flag_bucket" {
  bucket = "wpctf-2025-flag-s3-bucket-0197643419247650"
  force_destroy = true

  tags = {
    Name = "CTFFlagBucket"
  }
}

resource "aws_s3_object" "flag_object" {
  bucket = aws_s3_bucket.flag_bucket.id
  key    = "flag.txt"
  content = "WPCTF{_3v1l_l4t3_r4l_m0v_3m3nt_c0mpl_3t3d_!!!}"
}

resource "random_string" "suffix" {
  length  = 6
  special = false
  upper   = false
  lower   = true
  numeric = true
}

data "archive_file" "lambda_zip" {
  type        = "zip"
  source_dir  = "${path.module}/lambda"
  output_path = "${path.module}/lambda_payload.zip"
}

resource "aws_lambda_function" "ssti_lambda" {
  function_name = "SSTILambda"
  handler       = "lambda_function.lambda_handler"
  role          = aws_iam_role.lambda_role.arn
  runtime       = "python3.9"

  filename = data.archive_file.lambda_zip.output_path

  environment {
    variables = {
      FLAG_BUCKET_NAME = aws_s3_bucket.flag_bucket.bucket
    }
  }

  tags = {
    Name = "SSTILambda"
  }
}

resource "aws_lambda_function_url" "lambda_url" {
  function_name       = aws_lambda_function.ssti_lambda.function_name
  authorization_type  = "NONE"
}

output "lambda_function_url" {
  value = aws_lambda_function_url.lambda_url.function_url
}