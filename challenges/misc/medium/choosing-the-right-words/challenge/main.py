from fastapi import FastAPI, Request
from slowapi import Limiter
from slowapi.util import get_remote_address
from slowapi.errors import RateLimitExceeded
from fastapi.responses import JSONResponse
from slowapi.middleware import SlowAPIMiddleware
import skops.io as sio
from pydantic import BaseModel
import os
import pandas as pd

EMAILS_PATH = os.environ.get("EMAILS_PATH", "spam_emails.csv")
FLAG = os.environ.get("FLAG", "WPCTF{REDACTED}")

class AncientWords(BaseModel):
    sentence: str


# Set up the Limiter with a global rate limit
limiter = Limiter(
    key_func=get_remote_address,
    default_limits=["1/minute"]
)


# Load the model, which predict with 99% accuracy the spam emails
clf = sio.load("model.skops")

# Load the emails
emails_df = pd.read_csv(EMAILS_PATH)
emails = emails_df['text'].tolist()

# Extract all email words
all_email_words = set()
for email in emails:
    words = email.lower().split()
    all_email_words.update(words)

app = FastAPI()

# Attach the limiter to the app state
app.state.limiter = limiter

# Add the rate limiting middleware
app.add_middleware(SlowAPIMiddleware)

# Optional: Handle RateLimitExceeded error
@app.exception_handler(RateLimitExceeded)
async def rate_limit_exceeded_handler(request: Request, exc: RateLimitExceeded):
    return JSONResponse(
        status_code=429,
        content={"detail": "Global rate limit exceeded. Please try again later."}
    )

@app.post("/some-ancient-words")
async def some_ancient_words(data: AncientWords):
    # Check if the emails contains words not found in the original spam emails
    input_words = set(data.sentence.lower().split())

    if len(input_words.difference(all_email_words)) > 0:
        return {"quote": "Silence is better than unmeaning words. (Pythagoras)"}
    
    # Check the poisoned words are at least 30
    if len(input_words) < 30:
        return {"quote": "Raise your word, not your voice. It is rain that grows flowers, not thunder. (Rumi)"}

    emails_to_be_classified = [email + " " + data.sentence for email in emails]

    predictions = clf.predict(emails_to_be_classified)
    spam_ratio = sum(predictions)/len(predictions)

    if round(spam_ratio) <= 0.3:
        return {
            "flag": FLAG, 
            "quote": "Kind words do not cost much. Yet they accomplish much. (Blaise Pascal)"
            }

    # Not the right words, I would say
    return {"quote": "The difference between the almost right word and the right word is really a large matter—'tis the difference between the lightning-bug and the lightning (Mark Twain)"}


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)