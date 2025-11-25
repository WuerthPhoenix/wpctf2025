# %% [markdown]
# # Training an SVM model to detect spam emails

# %% [markdown]
# The goal of this notebook is to train a simple SVM model to be able to distinguish between spam and ham emails

# %% [markdown]
# We start by importing some useful libraries

# %%
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.feature_extraction.text import TfidfVectorizer
import numpy as np
from sklearn.pipeline import make_pipeline
from sklearn.linear_model import SGDClassifier
from sklearn.preprocessing import MaxAbsScaler
from skops.io import dump

# %% [markdown]
# ## Data preprocessing

# %% [markdown]
# Let's start the preprocessing by reading the data

# %%
df = pd.read_csv('datasets/emails.csv')
x = df.text
y = df.label

# %% [markdown]
# Separate the dataset in three parts: 
# 
# - Training set
# - Test set
# - Attacking set

# %%
x_train_val, x_test, y_train_val, y_test = train_test_split(x, y, test_size=0.2, random_state=99, stratify=y)

x_train, x_val, y_train, y_val = train_test_split(x_train_val, y_train_val, test_size=0.2, random_state=99, stratify=y_train_val)

# %% [markdown]
# ## From words to numbers

# %% [markdown]
# To train the SVM we a numerical representation of our emails, for this reason we use [TF-IDF](https://medium.com/analytics-vidhya/tf-idf-term-frequency-technique-easiest-explanation-for-text-classification-in-nlp-with-code-8ca3912e58c3) to bring them to a more meaningful representation.

# %%
clf = make_pipeline(
    TfidfVectorizer(),
    MaxAbsScaler(),
    SGDClassifier(max_iter=1000, tol=1e-3, loss="log_loss")
)

# %%
clf.fit(x_train, y_train)

# %%
clf.predict([x_train[0]])

# %%
# predict x_test and evaluate the model
y_pred = clf.predict(x_test)
from sklearn.metrics import classification_report, accuracy_score
print(classification_report(y_test, y_pred))
print("Accuracy:", accuracy_score(y_test, y_pred))

# %%
spam_emails = x_val[y_val == 1]

# 2. Create labels
spam_labels = np.ones(len(spam_emails), dtype=int)   # label 1 for spam


shuffled_spam_df = spam_emails.sample(frac=1, random_state=42).reset_index(drop=True)

shuffled_spam_df[:200].to_csv("../challenge/spam_emails.csv", index=False)

# %%
dump(clf, '../challenge/model.skops')


