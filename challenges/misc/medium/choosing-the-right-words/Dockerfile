# Base image with Python 3.12
FROM python:3.12-slim@sha256:abc799c7ee22b0d66f46c367643088a35e048bbabd81212d73c2323aed38c64f

ARG FLAG=WPCTF{REDACTED}
ENV FLAG=${FLAG}

# Create a working directory inside the container
WORKDIR /app

# Copy local files to the container
COPY challenge /app

RUN pip install --no-cache-dir -r requirements.txt

# Run your challenge script
CMD ["python", "main.py"]
EXPOSE 8000
