# Use a lightweight Python image
FROM python:3.12.8-slim-bookworm

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

# Set a non-root user for better security
RUN useradd -m appuser

# Set the working directory
WORKDIR /app

# Copy dependencies file
COPY requirements.txt /app/

# Install Python dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY . /app/

# Set permissions for the application folder
RUN chown -R appuser:appuser /app

# Switch to the non-root user
USER appuser

# Expose the application port
EXPOSE 3924

# Run the application with production-ready Uvicorn
CMD ["uvicorn", "opsie:app", "--host", "0.0.0.0", "--port", "3924", "--workers", "2"]
