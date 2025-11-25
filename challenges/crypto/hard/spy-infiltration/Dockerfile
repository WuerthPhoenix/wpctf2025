FROM python:3.11-slim-bookworm

# Set environment variables to optimize Python behavior
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV DEBIAN_FRONTEND=noninteractive

# Create a non-root user and set the working directory
RUN useradd --create-home --shell /bin/bash appuser
WORKDIR /home/appuser/app

# Pass the flag securely at runtime
ARG FLAG=WPCTF{REDACTED}
ENV FLAG=${FLAG}

ARG SECRET="thisisverysecure"
ENV SECRET=${SECRET}

# Install dependencies
COPY --chown=appuser:appuser requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy the application files
COPY --chown=appuser:appuser main.py .
COPY --chown=appuser:appuser static ./static
COPY --chown=appuser:appuser templates ./templates

RUN chown appuser:appuser /home/appuser/app

# Switch to the non-root user
USER appuser

# Expose the application port
EXPOSE 5000

# Run the application using Gunicorn
CMD ["python3", "main.py"]
