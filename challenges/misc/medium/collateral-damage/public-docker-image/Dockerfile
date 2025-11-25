##################à
# Use a lightweight base image
FROM alpine:3.21.2

# Set environment variables
ENV S3_BUCKET_NAME=wpctf-2025-flag-s3-bucket-0197643419247650 \
    AWS_SERVICE=lambda \
    LAMBDA_URL=https://6ulgiro7h3ziyzyflmjmi6yvda0rpejm.lambda-url.eu-west-1.on.aws/

# Install necessary tools (curl and bash)
RUN apk add --no-cache curl bash

# Add a script to make the required curl request
RUN echo -e '#!/bin/bash\n' \
    'response=$(curl -X GET -H "Content-Type: application/json" -d "{}" "${LAMBDA_URL}health")\n' \
    'echo "Response from Lambda URL: $response"' > /entrypoint.sh && \
    chmod +x /entrypoint.sh

# Use Unix line endings
RUN dos2unix /entrypoint.sh || true

# Set the script as the container's entry point
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]

# Add metadata for clarity
LABEL maintainer="JEFF <jeff.bugged@megacorp.com>" \
      description="Lightweight and secure container with environment variables for AWS Lambda and a health check curl script."

