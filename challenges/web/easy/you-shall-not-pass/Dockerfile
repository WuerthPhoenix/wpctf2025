FROM python:3.11-slim

ARG FLAG=WPCTF{redacted}

WORKDIR /the-secret-invitation

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Initialize the database
COPY init_db.py .
COPY init.sql .
RUN sed -i "s/@@FLAG@@/${FLAG}/g" init.sql
RUN touch database.db && \
    python3 init_db.py && \
    rm init_db.py init.sql

RUN useradd -m ShadowPulse
USER ShadowPulse

# Copy application files
COPY app.py .
COPY static ./static
COPY templates ./templates

EXPOSE 4759

CMD ["python3", "app.py"]
